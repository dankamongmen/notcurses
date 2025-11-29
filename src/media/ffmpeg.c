#include "builddef.h"
#ifdef USE_FFMPEG
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include <libswscale/version.h>
#include <libswresample/swresample.h>
#include <libswresample/version.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include "lib/visual-details.h"
#include "lib/internal.h"

struct AVFormatContext;

// Simple audio logging function - writes to /tmp/ncplayer_audio.log
static void audio_log(const char* fmt, ...){
  FILE* logfile = fopen("/tmp/ncplayer_audio.log", "a");
  if(!logfile){
    return;
  }
  va_list args;
  va_start(args, fmt);
  vfprintf(logfile, fmt, args);
  va_end(args);
  fflush(logfile);
  fclose(logfile);
}
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;

#define AUDIO_PACKET_QUEUE_SIZE 8

typedef struct audio_packet_queue {
  AVPacket* packets[AUDIO_PACKET_QUEUE_SIZE];
  int head;
  int tail;
  int count;
} audio_packet_queue;

typedef struct ncvisual_details {
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;     // video codec context
  struct AVCodecContext* subtcodecctx; // subtitle codec context
  struct AVCodecContext* audiocodecctx; // audio codec context
  struct AVFrame* frame;               // frame as read/loaded/converted
  struct AVFrame* audio_frame;         // decoded audio frame
  struct AVCodec* codec;
  struct AVCodec* subtcodec;
  struct AVCodec* audiocodec;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  struct SwsContext* rgbactx;
  struct SwrContext* swrctx;           // audio resampler context
  int audio_out_channels;              // output channel count for resampler (1 or 2)
  AVSubtitle subtitle;
  int stream_index;        // match against this following av_read_frame()
  int sub_stream_index;    // subtitle stream index, can be < 0 if no subtitles
  int audio_stream_index;  // audio stream index, can be < 0 if no audio
  bool packet_outstanding;
  bool audio_packet_outstanding; // whether we have an audio packet waiting to be decoded
  audio_packet_queue pending_audio_packets; // audio packets waiting to be sent
  int64_t last_audio_frame_pts; // PTS of last processed audio frame (to prevent reprocessing)
  pthread_mutex_t packet_mutex;  // mutex for thread-safe packet reading
  pthread_mutex_t audio_packet_mutex; // mutex for audio packet queue
} ncvisual_details;

#define AUDIO_LOG_QUEUE_OPS 1

static void
audio_queue_init(audio_packet_queue* q){
  memset(q, 0, sizeof(*q));
}

static void
audio_queue_clear(audio_packet_queue* q){
  while(q->count > 0){
    av_packet_free(&q->packets[q->head]);
    q->packets[q->head] = NULL;
    q->head = (q->head + 1) % AUDIO_PACKET_QUEUE_SIZE;
    q->count--;
  }
  q->head = q->tail = 0;
}

static int
audio_queue_enqueue(audio_packet_queue* q, const AVPacket* pkt){
  if(q->count >= AUDIO_PACKET_QUEUE_SIZE){
    return -1;
  }
  AVPacket* copy = av_packet_alloc();
  if(!copy){
    return -1;
  }
  if(av_packet_ref(copy, pkt) < 0){
    av_packet_free(&copy);
    return -1;
  }
  q->packets[q->tail] = copy;
  q->tail = (q->tail + 1) % AUDIO_PACKET_QUEUE_SIZE;
  q->count++;
  return 0;
}

static AVPacket*
audio_queue_peek(audio_packet_queue* q){
  if(q->count == 0){
    return NULL;
  }
  return q->packets[q->head];
}

static void
audio_queue_pop(audio_packet_queue* q){
  if(q->count == 0){
    return;
  }
  av_packet_free(&q->packets[q->head]);
  q->packets[q->head] = NULL;
  q->head = (q->head + 1) % AUDIO_PACKET_QUEUE_SIZE;
  q->count--;
}

static void
ffmpeg_drain_pending_audio_locked(ncvisual_details* deets){
  if(!deets->audiocodecctx){
    audio_queue_clear(&deets->pending_audio_packets);
    deets->audio_packet_outstanding = false;
    return;
  }
  while(deets->pending_audio_packets.count > 0){
    AVPacket* pkt = audio_queue_peek(&deets->pending_audio_packets);
    if(!pkt){
      break;
    }
    int ret = avcodec_send_packet(deets->audiocodecctx, pkt);
    if(ret == 0){
      audio_queue_pop(&deets->pending_audio_packets);
      deets->audio_packet_outstanding = true;
      continue;
    }else if(ret == AVERROR(EAGAIN)){
      // Decoder still full; keep outstanding flag set because queue still populated.
      deets->audio_packet_outstanding = true;
      return;
    }else{
      // Drop problematic packet and keep trying remaining ones.
      audio_queue_pop(&deets->pending_audio_packets);
    }
  }
  if(deets->pending_audio_packets.count == 0){
    deets->audio_packet_outstanding = false;
  }
}

#define IMGALLOCALIGN 64

uint64_t ffmpeg_pkt_duration(const AVFrame* frame){
#if LIBAVUTIL_VERSION_MAJOR < 58
      return frame->pkt_duration;
#else
      return frame->duration;
#endif
}

/*static void
print_frame_summary(const AVCodecContext* cctx, const AVFrame* f){
  if(f == NULL){
    fprintf(stderr, "NULL frame\n");
    return;
  }
  char pfmt[128];
  av_get_pix_fmt_string(pfmt, sizeof(pfmt), f->format);
  fprintf(stderr, "Frame %05d %p (%d? %d?) %dx%d pfmt %d (%s)\n",
          cctx ? cctx->frame_number : 0, f,
          f->coded_picture_number,
          f->display_picture_number,
          f->width, f->height,
          f->format, pfmt);
  fprintf(stderr, " Data (%d):", AV_NUM_DATA_POINTERS);
  int i;
  for(i = 0 ; i < AV_NUM_DATA_POINTERS ; ++i){
    fprintf(stderr, " %p", f->data[i]);
  }
  fprintf(stderr, "\n Linesizes:");
  for(i = 0 ; i < AV_NUM_DATA_POINTERS ; ++i){
    fprintf(stderr, " %d", f->linesize[i]);
  }
  if(f->sample_aspect_ratio.num == 0 && f->sample_aspect_ratio.den == 1){
    fprintf(stderr, "\n Aspect ratio unknown");
  }else{
    fprintf(stderr, "\n Aspect ratio %d:%d", f->sample_aspect_ratio.num, f->sample_aspect_ratio.den);
  }
  if(f->interlaced_frame){
    fprintf(stderr, " [ILaced]");
  }
  if(f->palette_has_changed){
    fprintf(stderr, " [NewPal]");
  }
  fprintf(stderr, " PTS %" PRId64 " Flags: 0x%04x\n", f->pts, f->flags);
  fprintf(stderr, " %" PRIu64 "ms@%" PRIu64 "ms (%skeyframe) qual: %d\n",
          ffmpeg_pkt_duration(f), // FIXME in 'time_base' units
          f->best_effort_timestamp,
          f->key_frame ? "" : "non-",
          f->quality);
}*/

static char*
deass(const char* ass){
  // SSA/ASS formats:
  // Dialogue: Marked=0,0:02:40.65,0:02:41.79,Wolf main,Cher,0000,0000,0000,,Et les enregistrements de ses ondes delta ?
  // FIXME more
  if(strncmp(ass, "Dialogue:", strlen("Dialogue:"))){
    return NULL;
  }
  const char* delim = strchr(ass, ',');
  int commas = 0; // we want 8
  while(delim && commas < 8){
    delim = strchr(delim + 1, ',');
    ++commas;
  }
  if(!delim){
    return NULL;
  }
  // handle ASS syntax...\i0, \b0, etc.
  char* dup = strdup(delim + 1);
  char* c = dup;
  while(*c){
    if(*c == '\\'){
      *c = ' ';
      ++c;
      if(*c){
        *c = ' ';;
      }
    }
    ++c;
  }
  return dup;
}

static struct ncplane*
subtitle_plane_from_text(ncplane* parent, const char* text){
  if(parent == NULL){
//logerror("need a parent plane\n");
    return NULL;
  }
  int width = ncstrwidth(text, NULL, NULL);
  if(width <= 0){
//logwarn("couldn't extract subtitle from %s\n", text);
    return NULL;
  }
  int rows = (width + ncplane_dim_x(parent) - 1) / ncplane_dim_x(parent);
  struct ncplane_options nopts = {
    .y = ncplane_dim_y(parent) - (rows + 1),
    .rows = rows,
    .cols = ncplane_dim_x(parent),
    .name = "subt",
  };
  struct ncplane* n = ncplane_create(parent, &nopts);
  if(n == NULL){
//logerror("error creating subtitle plane\n");
    return NULL;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_HIGHCONTRAST);
  ncchannels_set_fg_rgb8(&channels, 0x88, 0x88, 0x88);
  ncplane_stain(n, -1, -1, 0, 0, channels, channels, channels, channels);
  ncchannels_set_fg_default(&channels);
  ncplane_puttext(n, 0, NCALIGN_LEFT, text, NULL);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  return n;
}

static uint32_t palette[NCPALETTESIZE];

struct ncplane* ffmpeg_subtitle(ncplane* parent, const ncvisual* ncv){
  for(unsigned i = 0 ; i < ncv->details->subtitle.num_rects ; ++i){
    // it is possible that there are more than one subtitle rects present,
    // but we only bother dealing with the first one we find FIXME?
    const AVSubtitleRect* rect = ncv->details->subtitle.rects[i];
    if(rect->type == SUBTITLE_ASS){
      char* ass = deass(rect->ass);
      struct ncplane* n = NULL;
      if(ass){
        n = subtitle_plane_from_text(parent, ass);
      }
      free(ass);
      return n;
    }else if(rect->type == SUBTITLE_TEXT){;
      return subtitle_plane_from_text(parent, rect->text);
    }else if(rect->type == SUBTITLE_BITMAP){
      // there are technically up to AV_NUM_DATA_POINTERS planes, but we
      // only try to work with the first FIXME?
      if(rect->linesize[0] != rect->w){
//logwarn("bitmap subtitle size %d != width %d\n", rect->linesize[0], rect->w);
        continue;
      }
      struct notcurses* nc = ncplane_notcurses(parent);
      const unsigned cellpxy = ncplane_pile_const(parent)->cellpxy;
      const unsigned cellpxx = ncplane_pile_const(parent)->cellpxx;
      if(cellpxy <= 0 || cellpxx <= 0){
        continue;
      }
      struct ncvisual* v = ncvisual_from_palidx(rect->data[0], rect->h,
                                                rect->w, rect->w,
                                                NCPALETTESIZE, 1, palette);
      if(v == NULL){
        return NULL;
      }
      int rows = (rect->h + cellpxx - 1) / cellpxy;
      struct ncplane_options nopts = {
        .rows = rows,
        .cols = (rect->w + cellpxx - 1) / cellpxx,
        .y = ncplane_dim_y(parent) - rows - 1,
        .name = "t1st",
      };
      struct ncplane* vn = ncplane_create(parent, &nopts);
      if(vn == NULL){
        ncvisual_destroy(v);
        return NULL;
      }
      struct ncvisual_options vopts = {
        .n = vn,
        .blitter = NCBLIT_PIXEL,
        .scaling = NCSCALE_STRETCH,
      };
      if(ncvisual_blit(nc, v, &vopts) == NULL){
        ncplane_destroy(vn);
        ncvisual_destroy(v);
        return NULL;
      }
      ncvisual_destroy(v);
      return vn;
    }
  }
  return NULL;
}

static int
averr2ncerr(int averr){
  if(averr == AVERROR_EOF){
    return 1;
  }
  // FIXME need to map averror codes to ncerrors
//fprintf(stderr, "AVERR: %d/%x %d/%x\n", averr, averr, -averr, -averr);
  return -1;
}

// force an AVImage to RGBA for safe use with the ncpixel API
static int
force_rgba(ncvisual* n){
  const int targformat = AV_PIX_FMT_RGBA;
  AVFrame* inf = n->details->frame;
//fprintf(stderr, "%p got format: %d (%d/%d) want format: %d (%d/%d)\n", n->details->frame, inf->format, n->pixy, n->pixx, targformat);
  if(inf->format == targformat){
    return 0;
  }
  AVFrame* sframe = av_frame_alloc();
  if(sframe == NULL){
//fprintf(stderr, "Couldn't allocate output frame for scaled frame\n");
    return -1;
  }
//fprintf(stderr, "WHN NCV: %d/%d\n", inf->width, inf->height);
  n->details->rgbactx = sws_getCachedContext(n->details->rgbactx,
                                            inf->width, inf->height, inf->format,
                                            inf->width, inf->height, targformat,
                                            SWS_LANCZOS, NULL, NULL, NULL);
  if(n->details->rgbactx == NULL){
//fprintf(stderr, "Error retrieving details->rgbactx\n");
    return -1;
  }
  memcpy(sframe, inf, sizeof(*inf));
  sframe->format = targformat;
  sframe->width = inf->width;
  sframe->height = inf->height;
  int size = av_image_alloc(sframe->data, sframe->linesize,
                            sframe->width, sframe->height,
                            sframe->format,
                            IMGALLOCALIGN);
  if(size < 0){
//fprintf(stderr, "Error allocating visual data (%d X %d)\n", sframe->height, sframe->width);
    return -1;
  }
//fprintf(stderr, "INFRAME DAA: %p SDATA: %p FDATA: %p\n", inframe->data[0], sframe->data[0], ncv->details->frame->data[0]);
  int height = sws_scale(n->details->rgbactx, (const uint8_t* const*)inf->data,
                         inf->linesize, 0, inf->height, sframe->data,
                         sframe->linesize);
  if(height < 0){
//fprintf(stderr, "Error applying converting %d\n", inf->format);
    av_frame_free(&sframe);
    return -1;
  }
  int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(sframe->format));
  if(bpp != 32){
//fprintf(stderr, "Bad bits-per-pixel (wanted 32, got %d)\n", bpp);
    av_frame_free(&sframe);
    return -1;
  }
  n->rowstride = sframe->linesize[0];
  if((uint32_t*)sframe->data[0] != n->data){
//fprintf(stderr, "SETTING UP RESIZE %p\n", n->data);
    if(n->details->frame){
      if(n->owndata){
        // we don't free the frame data here, because it's going to be
        // freed (if appropriate) by ncvisual_set_data() momentarily.
        av_freep(&n->details->frame);
      }
    }
    ncvisual_set_data(n, sframe->data[0], true);
  }
  n->details->frame = sframe;
  return 0;
}

// turn arbitrary input packets into RGBA frames. reads packets until it gets
// a visual frame. a packet might contain several frames (this is typically
// true only of audio), and a frame might be carried across several packets.
// * avcodec_receive_frame() returns EAGAIN if it needs more packets.
// * avcodec_send_packet() returns EAGAIN if avcodec_receive_frame() needs
//    be called to extract further frames; in this case, the packet ought
//    be resubmitted once the existing frames are cleared.
static int
ffmpeg_decode(ncvisual* n){
  if(n->details->fmtctx == NULL){ // not a file-backed ncvisual
    return -1;
  }
  bool have_frame = false;
  bool unref = false;
  // note that there are two loops here; once we're out of the external one,
  // we've either returned a failure, or we have a frame. averr2ncerr()
  // translates AVERROR_EOF into a return of 1.
  do{
    if(!n->details->packet_outstanding){
      do{
        if(unref){
          av_packet_unref(n->details->packet);
        }
        int averr;
        // Single point for reading packets - no mutex needed as only video decoder reads
        averr = av_read_frame(n->details->fmtctx, n->details->packet);
        if(averr < 0){
          /*if(averr != AVERROR_EOF){
            fprintf(stderr, "Error reading frame info (%s)\n", av_err2str(averr));
          }*/
          return averr2ncerr(averr);
        }
        unref = true;

        // Handle subtitle packets
        if(n->details->packet->stream_index == n->details->sub_stream_index){
          int result = 0, ret;
          avsubtitle_free(&n->details->subtitle);
          ret = avcodec_decode_subtitle2(n->details->subtcodecctx, &n->details->subtitle, &result, n->details->packet);
          if(ret >= 0 && result){
            // FIXME?
          }
          continue; // Continue reading, this wasn't a video packet
        }

        // Handle audio packets - send them to audio decoder (audio thread will receive frames)
        if(n->details->packet->stream_index == n->details->audio_stream_index){
          if(n->details->audiocodecctx){
            // Validate packet and codec context before sending
            if(!n->details->packet || !n->details->audiocodecctx){
              av_packet_unref(n->details->packet);
              continue;
            }

            // Check packet validity - must have data and size
            if(n->details->packet->size <= 0 || !n->details->packet->data){
              // Empty or invalid packet - skip it
              av_packet_unref(n->details->packet);
              continue;
            }

            static int audio_packet_count = 0;
            audio_packet_count++;

            pthread_mutex_lock(&n->details->audio_packet_mutex);
            ffmpeg_drain_pending_audio_locked(n->details);

            // Now try to send the new packet
            int audio_ret = avcodec_send_packet(n->details->audiocodecctx, n->details->packet);
            if(audio_ret == 0){
              n->details->audio_packet_outstanding = true;
              if(audio_packet_count <= 10 || audio_packet_count % 100 == 0){
                audio_log("ffmpeg_decode: Sent audio packet %d successfully\n", audio_packet_count);
              }
              av_packet_unref(n->details->packet);
            }else if(audio_ret == AVERROR(EAGAIN)){
              // Decoder full - save this packet for next time
              if(audio_queue_enqueue(&n->details->pending_audio_packets, n->details->packet) == 0){
                n->details->audio_packet_outstanding = true;
                if(audio_packet_count <= 10 || audio_packet_count % 100 == 0){
                  audio_log("ffmpeg_decode: Audio decoder full (EAGAIN), queuing packet %d (queued=%d)\n",
                            audio_packet_count, n->details->pending_audio_packets.count);
                }
              }else{
                // Already have pending packet - drop this one (queue full)
                if(audio_packet_count <= 10 || audio_packet_count % 100 == 0){
                  audio_log("ffmpeg_decode: Audio queue full, dropping packet %d\n", audio_packet_count);
                }
              }
              av_packet_unref(n->details->packet);
            }else if(audio_ret == AVERROR_EOF){
              // EOF - this is normal at end of stream
              av_packet_unref(n->details->packet);
            }else{
              // Error - log it but don't crash
              if(audio_packet_count <= 10){
                audio_log("ffmpeg_decode: Error sending audio packet %d: %d (skipping)\n", audio_packet_count, audio_ret);
              }
              av_packet_unref(n->details->packet);
            }

            pthread_mutex_unlock(&n->details->audio_packet_mutex);
          }else{
            av_packet_unref(n->details->packet);
          }
          continue; // Continue reading, this wasn't a video packet
        }

        // This is a video packet - break out of loop to process it
        break;
      }while(1);
      n->details->packet_outstanding = true;
      int averr = avcodec_send_packet(n->details->codecctx, n->details->packet);
      if(averr < 0){
        n->details->packet_outstanding = false;
        av_packet_unref(n->details->packet);
  //fprintf(stderr, "Error processing AVPacket\n");
        return averr2ncerr(averr);
      }
    }
    int averr = avcodec_receive_frame(n->details->codecctx, n->details->frame);
    if(averr >= 0){
      have_frame = true;
    }else if(averr < 0){
      av_packet_unref(n->details->packet);
      have_frame = false;
      n->details->packet_outstanding = false;
      if(averr != AVERROR(EAGAIN)){
        return averr2ncerr(averr);
      }
    }
//fprintf(stderr, "Error decoding AVPacket\n");
  }while(!have_frame);
//print_frame_summary(n->details->codecctx, n->details->frame);
  const AVFrame* f = n->details->frame;
  n->rowstride = f->linesize[0];
  n->pixx = n->details->frame->width;
  n->pixy = n->details->frame->height;
//fprintf(stderr, "good decode! %d/%d %d %p\n", n->details->frame->height, n->details->frame->width, n->rowstride, f->data);
  ncvisual_set_data(n, f->data[0], false);
  force_rgba(n);
  return 0;
}

static ncvisual_details*
ffmpeg_details_init(void){
  ncvisual_details* deets = malloc(sizeof(*deets));
  if(deets){
    memset(deets, 0, sizeof(*deets));
    deets->stream_index = -1;
    deets->sub_stream_index = -1;
    deets->audio_stream_index = -1;
    deets->last_audio_frame_pts = AV_NOPTS_VALUE; // No frame processed yet
    deets->audio_out_channels = 0; // Will be set when resampler is initialized
    audio_queue_init(&deets->pending_audio_packets);
    if(pthread_mutex_init(&deets->packet_mutex, NULL) != 0){
      free(deets);
      return NULL;
    }
    if(pthread_mutex_init(&deets->audio_packet_mutex, NULL) != 0){
      pthread_mutex_destroy(&deets->packet_mutex);
      free(deets);
      return NULL;
    }
    if((deets->frame = av_frame_alloc()) == NULL){
      pthread_mutex_destroy(&deets->packet_mutex);
      free(deets);
      return NULL;
    }
    if((deets->audio_frame = av_frame_alloc()) == NULL){
      av_frame_free(&deets->frame);
      pthread_mutex_destroy(&deets->packet_mutex);
      free(deets);
      return NULL;
    }
  }
  return deets;
}

static ncvisual*
ffmpeg_create(){
  ncvisual* nc = malloc(sizeof(*nc));
  if(nc){
    memset(nc, 0, sizeof(*nc));
    if((nc->details = ffmpeg_details_init()) == NULL){
      free(nc);
      return NULL;
    }
  }
  return nc;
}

static ncvisual*
ffmpeg_from_file(const char* filename){
  ncvisual* ncv = ffmpeg_create();
  if(ncv == NULL){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    return NULL;
  }
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details->frame);
  int averr = avformat_open_input(&ncv->details->fmtctx, filename, NULL, NULL);
  if(averr < 0){
//fprintf(stderr, "Couldn't open %s (%d)\n", filename, averr);
    goto err;
  }
  averr = avformat_find_stream_info(ncv->details->fmtctx, NULL);
  if(averr < 0){
//fprintf(stderr, "Error extracting stream info from %s (%d)\n", filename, averr);
    goto err;
  }
//av_dump_format(ncv->details->fmtctx, 0, filename, false);
  if((averr = av_find_best_stream(ncv->details->fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1,
#if LIBAVFORMAT_VERSION_MAJOR >= 59
                                  (const AVCodec**)&ncv->details->subtcodec, 0)) >= 0){
#else
                                  &ncv->details->subtcodec, 0)) >= 0){
#endif
    ncv->details->sub_stream_index = averr;
    if((ncv->details->subtcodecctx = avcodec_alloc_context3(ncv->details->subtcodec)) == NULL){
      //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
      goto err;
    }
    // FIXME do we need avcodec_parameters_to_context() here?
    if(avcodec_open2(ncv->details->subtcodecctx, ncv->details->subtcodec, NULL) < 0){
      //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
      goto err;
    }
  }else{
    ncv->details->sub_stream_index = -1;
  }
  // Find audio stream (similar to subtitle detection)
  if((averr = av_find_best_stream(ncv->details->fmtctx, AVMEDIA_TYPE_AUDIO, -1, -1,
#if LIBAVFORMAT_VERSION_MAJOR >= 59
                                  (const AVCodec**)&ncv->details->audiocodec, 0)) >= 0){
#else
                                  &ncv->details->audiocodec, 0)) >= 0){
#endif
    ncv->details->audio_stream_index = averr;
    AVStream* ast = ncv->details->fmtctx->streams[ncv->details->audio_stream_index];
    if((ncv->details->audiocodecctx = avcodec_alloc_context3(ncv->details->audiocodec)) == NULL){
      goto err;
    }
    if(avcodec_parameters_to_context(ncv->details->audiocodecctx, ast->codecpar) < 0){
      goto err;
    }
    if(avcodec_open2(ncv->details->audiocodecctx, ncv->details->audiocodec, NULL) < 0){
      goto err;
    }
  }else{
    ncv->details->audio_stream_index = -1;
  }
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details->frame);
  if((ncv->details->packet = av_packet_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    goto err;
  }
  if((averr = av_find_best_stream(ncv->details->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1,
#if LIBAVFORMAT_VERSION_MAJOR >= 59
                                  (const AVCodec**)&ncv->details->codec, 0)) < 0){
#else
                                  &ncv->details->codec, 0)) < 0){
#endif
    // fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  ncv->details->stream_index = averr;
  if(ncv->details->codec == NULL){
    //fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  AVStream* st = ncv->details->fmtctx->streams[ncv->details->stream_index];
  if((ncv->details->codecctx = avcodec_alloc_context3(ncv->details->codec)) == NULL){
    //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    goto err;
  }
  if(avcodec_parameters_to_context(ncv->details->codecctx, st->codecpar) < 0){
    goto err;
  }
  if(avcodec_open2(ncv->details->codecctx, ncv->details->codec, NULL) < 0){
    //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details->frame);
  // frame is set up in prep_details(), so that format can be set there, as
  // is necessary when it is prepared from inputs other than files.
  if(ffmpeg_decode(ncv)){
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return NULL;
}

// Decode audio packet and return number of samples decoded
// Returns: >0 = samples decoded, 0 = need more data, <0 = error
// This function is safe to call - it doesn't interfere with video decoding
static int
ffmpeg_decode_audio_internal(ncvisual* ncv, AVPacket* packet){
  if(!ncv->details->audiocodecctx || ncv->details->audio_stream_index < 0){
    return -1;
  }

  // Send packet to decoder if provided
  if(packet && packet->stream_index == ncv->details->audio_stream_index){
    int averr = avcodec_send_packet(ncv->details->audiocodecctx, packet);
    if(averr < 0 && averr != AVERROR(EAGAIN) && averr != AVERROR_EOF){
      return averr2ncerr(averr);
    }
    ncv->details->audio_packet_outstanding = (averr == 0);
  }

  // If no packet outstanding, return 0 (need more data)
  if(!ncv->details->audio_packet_outstanding){
    return 0;
  }

  // Receive decoded frame
  int averr = avcodec_receive_frame(ncv->details->audiocodecctx, ncv->details->audio_frame);
  if(averr == 0){
    ncv->details->audio_packet_outstanding = false;
    return ncv->details->audio_frame->nb_samples;
  }else if(averr == AVERROR(EAGAIN)){
    return 0; // need more packets
  }else if(averr == AVERROR_EOF){
    return 1; // EOF
  }else{
    return averr2ncerr(averr);
  }
}

// Initialize audio resampler for converting to output format
// Returns 0 on success, <0 on error
// This function is safe to call - it doesn't interfere with video decoding
static int
ffmpeg_init_audio_resampler_internal(ncvisual* ncv, int out_sample_rate, int out_channels){
  if(!ncv->details->audiocodecctx || ncv->details->audio_stream_index < 0){
    return -1;
  }

  AVCodecContext* acodecctx = ncv->details->audiocodecctx;

  // Free existing resampler if any
  if(ncv->details->swrctx){
    swr_free(&ncv->details->swrctx);
  }

  // Use older API with channel masks (more stable)
  uint64_t in_channel_mask = 0;
  uint64_t out_channel_mask = 0;

  // Get input channel count
  int in_ch_count = 0;
  if(acodecctx->ch_layout.nb_channels > 0){
    in_ch_count = acodecctx->ch_layout.nb_channels;
  }else{
    // Fallback for older FFmpeg versions
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    in_ch_count = acodecctx->channels;
    #pragma GCC diagnostic pop
  }

  // Map channel count to standard layout masks
  if(in_ch_count == 1){
    in_channel_mask = AV_CH_LAYOUT_MONO;
  }else if(in_ch_count == 2){
    in_channel_mask = AV_CH_LAYOUT_STEREO;
  }else if(in_ch_count == 6){
    in_channel_mask = AV_CH_LAYOUT_5POINT1;
  }else if(in_ch_count == 4){
    in_channel_mask = AV_CH_LAYOUT_QUAD;
  }else if(in_ch_count == 8){
    in_channel_mask = AV_CH_LAYOUT_7POINT1;
  }else{
    // Default to stereo for unknown channel counts
    in_channel_mask = AV_CH_LAYOUT_STEREO;
    in_ch_count = 2;
  }

  out_channel_mask = (out_channels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  ncv->details->swrctx = swr_alloc_set_opts(
    NULL,
    out_channel_mask, AV_SAMPLE_FMT_S16, out_sample_rate,
    in_channel_mask, acodecctx->sample_fmt, acodecctx->sample_rate,
    0, NULL);
  #pragma GCC diagnostic pop

  if(!ncv->details->swrctx){
    return -1;
  }

  int ret = swr_init(ncv->details->swrctx);
  if(ret < 0){
    swr_free(&ncv->details->swrctx);
    return -1;
  }

  // Store output channel count for buffer size calculations
  ncv->details->audio_out_channels = out_channels;

  return 0;
}

// iterate over the decoded frames, calling streamer() with curry for each.
// frames carry a presentation time relative to the beginning, so we get an
// initial timestamp, and check each frame against the elapsed time to sync
// up playback.
static int
ffmpeg_stream(notcurses* nc, ncvisual* ncv, float timescale,
              ncstreamcb streamer, const struct ncvisual_options* vopts,
              void* curry){
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  uint64_t nsbegin = timespec_to_ns(&begin);
  //bool usets = false;
  // each frame has a duration in units of time_base. keep the aggregate, in case
  // we don't have PTS available.
  uint64_t sum_duration = 0;
  ncplane* newn = NULL;
  struct ncvisual_options activevopts;
  memcpy(&activevopts, vopts, sizeof(*vopts));
  int ncerr;
  do{
    // codecctx seems to be off by a factor of 2 regularly. instead, go with
    // the time_base from the avformatctx. except ts isn't properly reset for
    // all media when we loop =[. we seem to be accurate enough now with the
    // tbase/ppd. see https://github.com/dankamongmen/notcurses/issues/1352.
    double tbase = av_q2d(ncv->details->fmtctx->streams[ncv->details->stream_index]->time_base);
    if(isnan(tbase)){
      tbase = 0;
    }
    if(activevopts.n){
      ncplane_erase(activevopts.n); // new frame could be partially transparent
    }
    // decay the blitter explicitly, so that the callback knows the blitter it
    // was actually rendered with. basically just need rgba_blitter(), but
    // that's not exported.
    ncvgeom geom;
    ncvisual_geom(nc, ncv, &activevopts, &geom);
    activevopts.blitter = geom.blitter;
    if((newn = ncvisual_blit(nc, ncv, &activevopts)) == NULL){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return -1;
    }
    if(activevopts.n != newn){
      activevopts.n = newn;
    }
    // display duration in units of time_base
    const uint64_t pktduration = ffmpeg_pkt_duration(ncv->details->frame);
    uint64_t duration = pktduration * tbase * NANOSECS_IN_SEC;
    double schedns = nsbegin;
    sum_duration += (duration * timescale);
    schedns += sum_duration;
    struct timespec abstime;
    ns_to_timespec(schedns, &abstime);
    int r;
    if(streamer){
      r = streamer(ncv, &activevopts, &abstime, curry);
    }else{
      r = ncvisual_simple_streamer(ncv, &activevopts, &abstime, curry);
    }
    if(r){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return r;
    }
  }while((ncerr = ffmpeg_decode(ncv)) == 0);
  if(activevopts.n != vopts->n){
    ncplane_destroy(activevopts.n);
  }
  if(ncerr == 1){ // 1 indicates reaching EOF
    ncerr = 0;
  }
  return ncerr;
}

static int
ffmpeg_decode_loop(ncvisual* ncv){
  int r = ffmpeg_decode(ncv);
  if(r == 1){
    if(av_seek_frame(ncv->details->fmtctx, ncv->details->stream_index, 0, AVSEEK_FLAG_FRAME) < 0){
      // FIXME log error
      return -1;
    }
    if(ffmpeg_decode(ncv) < 0){
      return -1;
    }
  }
  return r;
}

// do a resize *without* updating the ncvisual structure. if the target
// parameters are already matched, the existing data will be returned.
// otherwise, a scaled copy will be returned. they can be differentiated by
// comparing the result against ncv->data.
static uint32_t*
ffmpeg_resize_internal(const ncvisual* ncv, int rows, int* stride, int cols,
                       const blitterargs* bargs){
  const AVFrame* inframe = ncv->details->frame;
//print_frame_summary(NULL, inframe);
  const int targformat = AV_PIX_FMT_RGBA;
//fprintf(stderr, "got format: %d (%d/%d) want format: %d (%d/%d)\n", inframe->format, inframe->height, inframe->width, targformat, rows, cols);
  // FIXME need account for beg{y,x} here, no? what if no inframe?
  if(!inframe || (cols == inframe->width && rows == inframe->height && inframe->format == targformat)){
    // no change necessary. return original data -- we don't duplicate.
    *stride = ncv->rowstride;
    return ncv->data;
  }
  const int srclenx = bargs->lenx ? bargs->lenx : inframe->width;
  const int srcleny = bargs->leny ? bargs->leny : inframe->height;
//fprintf(stderr, "src %d/%d -> targ %d/%d ctx: %p\n", srcleny, srclenx, rows, cols, ncv->details->swsctx);
  ncv->details->swsctx = sws_getCachedContext(ncv->details->swsctx,
                                              srclenx, srcleny,
                                              inframe->format,
                                              cols, rows, targformat,
                                              SWS_LANCZOS, NULL, NULL, NULL);
  if(ncv->details->swsctx == NULL){
//fprintf(stderr, "Error retrieving details->swsctx\n");
    return NULL;
  }
  // necessitated by ffmpeg AVPicture API
  uint8_t* dptrs[4];
  int dlinesizes[4];
  int size = av_image_alloc(dptrs, dlinesizes, cols, rows, targformat, IMGALLOCALIGN);
  if(size < 0){
//fprintf(stderr, "Error allocating visual data (%d X %d)\n", sframe->height, sframe->width);
    return NULL;
  }
//fprintf(stderr, "INFRAME DAA: %p SDATA: %p FDATA: %p to %d/%d\n", inframe->data[0], sframe->data[0], ncv->details->frame->data[0], sframe->height, sframe->width);
  const uint8_t* data[4] = { (uint8_t*)ncv->data, };
  int height = sws_scale(ncv->details->swsctx, data,
                         inframe->linesize, 0, srcleny, dptrs, dlinesizes);
  if(height < 0){
//fprintf(stderr, "Error applying scaling (%d X %d)\n", inframe->height, inframe->width);
    av_freep(&dptrs[0]);
    return NULL;
  }
//fprintf(stderr, "scaled %d/%d to %d/%d\n", ncv->pixy, ncv->pixx, rows, cols);
  *stride = dlinesizes[0]; // FIXME check for others?
  return (uint32_t*)dptrs[0];
}

// resize frame, converting to RGBA (if necessary) along the way
static int
ffmpeg_resize(ncvisual* n, unsigned rows, unsigned cols){
  struct blitterargs bargs = {0};
  int stride;
  void* data = ffmpeg_resize_internal(n, rows, &stride, cols, &bargs);
  if(data == n->data){ // no change, return
    return 0;
  }
  if(data == NULL){
    return -1;
  }
  AVFrame* inf = n->details->frame;
//fprintf(stderr, "WHN NCV: %d/%d %p\n", inf->width, inf->height, n->data);
  inf->width = cols;
  inf->height = rows;
  inf->linesize[0] = stride;
  n->rowstride = stride;
  n->pixy = rows;
  n->pixx = cols;
  ncvisual_set_data(n, data, true);
//fprintf(stderr, "SIZE SCALED: %d %d (%u)\n", n->details->frame->height, n->details->frame->width, n->details->frame->linesize[0]);
  return 0;
}

// rows/cols: scaled output geometry (pixels)
static int
ffmpeg_blit(const ncvisual* ncv, unsigned rows, unsigned cols, ncplane* n,
            const struct blitset* bset, const blitterargs* bargs){
  void* data;
  int stride = 0;
  data = ffmpeg_resize_internal(ncv, rows, &stride, cols, bargs);
  if(data == NULL){
    return -1;
  }
//fprintf(stderr, "WHN NCV: bargslen: %d/%d targ: %d/%d\n", bargs->leny, bargs->lenx, rows, cols);
  int ret = 0;
  if(rgba_blit_dispatch(n, bset, stride, data, rows, cols, bargs) < 0){
    ret = -1;
  }
  if(data != ncv->data){
    av_freep(&data); // &dptrs[0]
  }
  return ret;
}

static void
ffmpeg_details_seed(ncvisual* ncv){
  av_frame_unref(ncv->details->frame);
  memset(ncv->details->frame, 0, sizeof(*ncv->details->frame));
  ncv->details->frame->linesize[0] = ncv->rowstride;
  ncv->details->frame->width = ncv->pixx;
  ncv->details->frame->height = ncv->pixy;
  ncv->details->frame->format = AV_PIX_FMT_RGBA;
}

static int
ffmpeg_log_level(int level){
  switch(level){
    case NCLOGLEVEL_SILENT: return AV_LOG_QUIET;
    case NCLOGLEVEL_PANIC: return AV_LOG_PANIC;
    case NCLOGLEVEL_FATAL: return AV_LOG_FATAL;
    case NCLOGLEVEL_ERROR: return AV_LOG_ERROR;
    case NCLOGLEVEL_WARNING: return AV_LOG_WARNING;
    case NCLOGLEVEL_INFO: return AV_LOG_INFO;
    case NCLOGLEVEL_VERBOSE: return AV_LOG_VERBOSE;
    case NCLOGLEVEL_DEBUG: return AV_LOG_DEBUG;
    case NCLOGLEVEL_TRACE: return AV_LOG_TRACE;
    default: break;
  }
  fprintf(stderr, "Invalid log level: %d\n", level);
  return AV_LOG_TRACE;
}

static int
ffmpeg_init(int logl){
  av_log_set_level(ffmpeg_log_level(logl));
  avdevice_register_all();
  // FIXME could also use av_log_set_callback() and capture the message...
  return 0;
}

static void
ffmpeg_printbanner(fbuf* f){
  fbuf_printf(f, "avformat %u.%u.%u avutil %u.%u.%u swscale %u.%u.%u avcodec %u.%u.%u avdevice %u.%u.%u" NL,
              LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
              LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
              LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO,
              LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO,
              LIBAVDEVICE_VERSION_MAJOR, LIBAVDEVICE_VERSION_MINOR, LIBAVDEVICE_VERSION_MICRO);
}

static void
ffmpeg_details_destroy(ncvisual_details* deets){
  // avcodec_close() is deprecated; avcodec_free_context() suffices
  avcodec_free_context(&deets->subtcodecctx);
  avcodec_free_context(&deets->audiocodecctx);
  avcodec_free_context(&deets->codecctx);
  av_frame_free(&deets->frame);
  av_frame_free(&deets->audio_frame);
  audio_queue_clear(&deets->pending_audio_packets);
  av_frame_free(&deets->audio_frame);
  swr_free(&deets->swrctx);
  sws_freeContext(deets->rgbactx);
  sws_freeContext(deets->swsctx);
  av_packet_free(&deets->packet);
  avformat_close_input(&deets->fmtctx);
  avsubtitle_free(&deets->subtitle);
  pthread_mutex_destroy(&deets->audio_packet_mutex);
  pthread_mutex_destroy(&deets->packet_mutex);
  free(deets);
}

static void
ffmpeg_destroy(ncvisual* ncv){
  if(ncv){
    ffmpeg_details_destroy(ncv->details);
    if(ncv->owndata){
      free(ncv->data);
    }
    free(ncv);
  }
}

// Public API functions for audio handling (called from play.cpp)
// Define API macro for visibility export
#ifndef __MINGW32__
#define API __attribute__((visibility("default")))
#else
#define API __declspec(dllexport)
#endif

API void
ffmpeg_audio_request_packets(ncvisual* ncv){
  if(!ncv || !ncv->details){
    return;
  }
  pthread_mutex_lock(&ncv->details->audio_packet_mutex);
  ffmpeg_drain_pending_audio_locked(ncv->details);
  pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
}

// Check if the visual has an audio stream
API bool
ffmpeg_has_audio(ncvisual* ncv){
  return ncv && ncv->details && ncv->details->audio_stream_index >= 0 &&
         ncv->details->audiocodecctx != NULL;
}

// Decode audio packet (public wrapper)
API int
ffmpeg_decode_audio(ncvisual* ncv, AVPacket* packet){
  return ffmpeg_decode_audio_internal(ncv, packet);
}

// Initialize audio resampler (public wrapper)
API int
ffmpeg_init_audio_resampler(ncvisual* ncv, int out_sample_rate, int out_channels){
  return ffmpeg_init_audio_resampler_internal(ncv, out_sample_rate, out_channels);
}

// Try to get a decoded audio frame (packets are read by video decoder)
// Returns: >0 = samples decoded, 0 = no frame available, <0 = error
// This function is called by the audio thread - it doesn't read packets
// IMPORTANT: avcodec_receive_frame can only return a frame once per packet sent
API int
ffmpeg_get_decoded_audio_frame(ncvisual* ncv){
  if(!ncv || !ncv->details || ncv->details->audio_stream_index < 0){
    return -1;
  }

  // Try to receive a decoded frame (non-blocking)
  // This will only return a frame once per packet - subsequent calls return EAGAIN
  static int receive_call_count = 0;
  receive_call_count++;
  int pending_after = 0;
  bool outstanding = false;
  pthread_mutex_lock(&ncv->details->audio_packet_mutex);
  int averr = avcodec_receive_frame(ncv->details->audiocodecctx, ncv->details->audio_frame);
  pending_after = ncv->details->pending_audio_packets.count;
  outstanding = ncv->details->audio_packet_outstanding;
  if(averr == 0){
    static int frame_counter = 0;
    frame_counter++;
    if(pending_after == 0){
      ncv->details->audio_packet_outstanding = false;
      outstanding = false;
    }
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    if(receive_call_count <= 20 || receive_call_count % 100 == 0 || frame_counter % 50 == 0){
      if(frame_counter <= 20 || frame_counter % 200 == 0){
        audio_log("ffmpeg_get_decoded_audio_frame: Frame received, pending queue=%d, total_frames=%d\n",
                  pending_after, frame_counter);
      }
    }

    // Check if this is a new frame (different PTS) or the same one we already processed
    int64_t current_pts = ncv->details->audio_frame->pts;
    if(current_pts == ncv->details->last_audio_frame_pts && current_pts != AV_NOPTS_VALUE){
      // Same frame as before - don't process again
      // This can happen if avcodec_receive_frame is called multiple times
      // Note: avcodec_receive_frame should only return each frame once, so this
      // check is defensive. If we see this frequently, there's a bug elsewhere.
      if(receive_call_count <= 20){
        audio_log("ffmpeg_get_decoded_audio_frame: Duplicate PTS detected (call %d)\n", receive_call_count);
      }
      return 0;
    }
    ncv->details->last_audio_frame_pts = current_pts;
    if(receive_call_count <= 20 || receive_call_count % 100 == 0){
      audio_log("ffmpeg_get_decoded_audio_frame: Received frame (call %d, samples=%d, pts=%" PRId64 ")\n",
                receive_call_count, ncv->details->audio_frame->nb_samples, current_pts);
    }
    return ncv->details->audio_frame->nb_samples;
  }else if(averr == AVERROR(EAGAIN)){
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    // Need more packets - video decoder will provide them
    // This is normal - the decoder needs more packets before it can produce a frame
    (void)outstanding;
    return 0;
  }else if(averr == AVERROR_EOF){
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    audio_log("ffmpeg_get_decoded_audio_frame: EOF (call %d)\n", receive_call_count);
    return 1; // EOF
  }else{
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    audio_log("ffmpeg_get_decoded_audio_frame: Error %d (call %d)\n", averr, receive_call_count);
    return -1; // Error
  }
}

// Resample audio frame to output format
// Returns: number of samples output, <0 on error
// NOTE: This should be called immediately after ffmpeg_get_decoded_audio_frame returns >0
// The frame data is only valid until the next call to avcodec_receive_frame
API int
ffmpeg_resample_audio(ncvisual* ncv, uint8_t** out_data, int* out_samples){
  if(!ncv || !ncv->details || !ncv->details->swrctx || !ncv->details->audio_frame){
    return -1;
  }

  AVFrame* frame = ncv->details->audio_frame;
  if(frame->nb_samples <= 0){
    return 0;
  }

  // Calculate output buffer size
  int64_t out_count = swr_get_out_samples(ncv->details->swrctx, frame->nb_samples);
  if(out_count < 0){
    return -1;
  }

  // Use OUTPUT channel count (from resampler), not input channel count
  int out_channels = ncv->details->audio_out_channels;
  if(out_channels <= 0){
    // Fallback: use input channel count, but limit to 2
    out_channels = ncv->details->audiocodecctx->ch_layout.nb_channels;
    if(out_channels == 0){
      // Fallback for older FFmpeg
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      out_channels = ncv->details->audiocodecctx->channels;
      #pragma GCC diagnostic pop
    }
    // Limit to stereo max
    if(out_channels > 2){
      out_channels = 2;
    }
  }

  int out_size = av_samples_get_buffer_size(NULL, out_channels, out_count,
                                            AV_SAMPLE_FMT_S16, 1);
  if(out_size < 0){
    return -1;
  }

  // Allocate output buffer
  *out_data = malloc(out_size);
  if(!*out_data){
    return -1;
  }

  // Perform resampling
  uint8_t* out_ptr = *out_data;
  int samples_out = swr_convert(ncv->details->swrctx, &out_ptr, out_count,
                                (const uint8_t**)frame->data, frame->nb_samples);

  if(samples_out < 0){
    free(*out_data);
    *out_data = NULL;
    return -1;
  }

  *out_samples = samples_out;
  return samples_out * out_channels * sizeof(int16_t); // Return bytes
}

// Get the decoded audio frame (for getting sample rate, channels, etc.)
API AVFrame*
ffmpeg_get_audio_frame(ncvisual* ncv){
  if(!ncv || !ncv->details){
    return NULL;
  }
  return ncv->details->audio_frame;
}

// Get audio sample rate from codec context
API int
ffmpeg_get_audio_sample_rate(ncvisual* ncv){
  if(!ncv || !ncv->details || !ncv->details->audiocodecctx){
    return 44100; // Default
  }
  return ncv->details->audiocodecctx->sample_rate > 0 ?
         ncv->details->audiocodecctx->sample_rate : 44100;
}

// Get audio channel count from codec context
API int
ffmpeg_get_audio_channels(ncvisual* ncv){
  if(!ncv || !ncv->details || !ncv->details->audiocodecctx){
    return 2; // Default stereo
  }
  AVCodecContext* acodecctx = ncv->details->audiocodecctx;
  int channels = acodecctx->ch_layout.nb_channels;
  if(channels == 0){
    // Fallback for older FFmpeg
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    channels = acodecctx->channels;
    #pragma GCC diagnostic pop
  }
  return channels > 0 ? channels : 2; // Default stereo
}

ncvisual_implementation local_visual_implementation = {
  .visual_init = ffmpeg_init,
  .visual_printbanner = ffmpeg_printbanner,
  .visual_blit = ffmpeg_blit,
  .visual_create = ffmpeg_create,
  .visual_from_file = ffmpeg_from_file,
  .visual_details_seed = ffmpeg_details_seed,
  .visual_decode = ffmpeg_decode,
  .visual_decode_loop = ffmpeg_decode_loop,
  .visual_stream = ffmpeg_stream,
  .visual_subtitle = ffmpeg_subtitle,
  .visual_resize = ffmpeg_resize,
  .visual_destroy = ffmpeg_destroy,
  .rowalign = 64, // ffmpeg wants multiples of IMGALIGN (64)
  .canopen_images = true,
  .canopen_videos = true,
};

#endif
