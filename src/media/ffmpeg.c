#include "builddef.h"
#ifdef USE_FFMPEG
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
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
#include <libavutil/cpu.h>
#include <unistd.h>
#include "lib/visual-details.h"
#include "lib/internal.h"
#include "lib/logging.h"
#include <notcurses/api.h>

#define SUBLOG_INFO(fmt, ...)  loginfo("[subtitle] " fmt, ##__VA_ARGS__)
#define SUBLOG_DEBUG(fmt, ...) logdebug("[subtitle] " fmt, ##__VA_ARGS__)
#define SUBLOG_WARN(fmt, ...)  logwarn("[subtitle] " fmt, ##__VA_ARGS__)

struct AVFormatContext;

struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;

double ffmpeg_get_video_position_seconds(const ncvisual* ncv);

#define AUDIO_PACKET_QUEUE_SIZE 8

typedef struct audio_packet_queue {
  AVPacket* packets[AUDIO_PACKET_QUEUE_SIZE];
  int head;
  int tail;
  int count;
} audio_packet_queue;

static int
ass_detect_text_field(const uint8_t* header, size_t size){
  if(!header || size == 0){
    return 9;
  }
  const char* data = (const char*)header;
  const char* end = data + size;
  const int default_index = 9;
  bool in_events = false;
  while(data < end){
    const char* line_start = data;
    const char* linebreak = memchr(line_start, '\n', end - line_start);
    size_t linelen = linebreak ? (size_t)(linebreak - line_start) : (size_t)(end - line_start);
    if(linelen >= 8 && !strncasecmp(line_start, "[Events]", 8)){
      in_events = true;
    }else if(in_events && linelen >= 7 && !strncasecmp(line_start, "Format:", 7)){
      const char* cursor = line_start + 7;
      while(cursor < line_start + linelen && isspace((unsigned char)*cursor)){
        ++cursor;
      }
      int field = 0;
      const char* token = cursor;
      for(const char* p = cursor ; p <= line_start + linelen ; ++p){
        if(p == line_start + linelen || *p == ',' || *p == '\r'){
          const char* t = token;
          while(t < p && isspace((unsigned char)*t)){
            ++t;
          }
          const char* q = p;
          while(q > t && isspace((unsigned char)*(q - 1))){
            --q;
          }
          if(q > t){
            size_t toklen = q - t;
            if(!strncasecmp(t, "Text", toklen)){
              return field;
            }
          }
          ++field;
          token = p + 1;
        }
      }
      return default_index;
    }
    data = linebreak ? linebreak + 1 : end;
  }
  return default_index;
}

static int
ffmpeg_detect_thread_count(void){
  static int cached = 0;
  if(cached > 0){
    return cached;
  }
  int threads = 0;
  const char* env = getenv("NCPLAYER_FFMPEG_THREADS");
  if(env && *env){
    char* endptr = NULL;
    long parsed = strtol(env, &endptr, 10);
    if(endptr != env && parsed > 0 && parsed < INT_MAX){
      threads = (int)parsed;
    }
  }
  if(threads <= 0){
    int avcpus = av_cpu_count();
    if(avcpus > 0){
      threads = avcpus;
    }else{
#ifdef _SC_NPROCESSORS_ONLN
      long syscpus = sysconf(_SC_NPROCESSORS_ONLN);
      if(syscpus > 0 && syscpus < INT_MAX){
        threads = (int)syscpus;
      }
#endif
    }
  }
  if(threads <= 0){
    threads = 1;
  }
  cached = threads;
  return cached;
}

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
  bool subtitle_active;
  double subtitle_start_time;
  double subtitle_end_time;
  double subtitle_time_base;
  int subtitle_text_field;
  bool subtitle_logged;
  int64_t subtitle_last_hash;
  char* subtitle_cached_text;
  int stream_index;        // match against this following av_read_frame()
  int sub_stream_index;    // subtitle stream index, can be < 0 if no subtitles
  int audio_stream_index;  // audio stream index, can be < 0 if no audio
  bool packet_outstanding;
  bool audio_packet_outstanding; // whether we have an audio packet waiting to be decoded
  audio_packet_queue pending_audio_packets; // audio packets waiting to be sent
  int64_t last_audio_frame_pts; // PTS of last processed audio frame (to prevent reprocessing)
  int64_t last_video_frame_pts; // PTS of last displayed video frame
  int64_t decoded_frames;
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

static struct ncplane*
subtitle_plane_from_text(ncplane* parent, const char* text, bool* logged_flag){
  if(parent == NULL || text == NULL){
    return NULL;
  }
  char* dup = strdup(text);
  if(!dup){
    return NULL;
  }
  char* trimmed = dup;
  const char* raw = dup;
  int commas = 0;
  for(; *trimmed ; ++trimmed){
    if(*trimmed == ','){
      ++commas;
      if(commas == 8){
        ++trimmed;
        break;
      }
    }
  }
  while(*trimmed && isspace((unsigned char)*trimmed)){
    ++trimmed;
  }
  size_t len = strlen(trimmed);
  while(len > 0 && isspace((unsigned char)trimmed[len - 1])){
    trimmed[--len] = '\0';
  }
  if(len == 0){
    free(dup);
    return NULL;
  }
  int linecount = 1;
  for(size_t i = 0 ; i < len ; ++i){
    if(trimmed[i] == '\r'){
      trimmed[i] = '\n';
    }
    if(trimmed[i] == '\n'){
      ++linecount;
    }
  }

  int parent_cols = ncplane_dim_x(parent);
  if(parent_cols <= 0){
    free(dup);
    return NULL;
  }
  int maxwidth = 0;
  char* walker = trimmed;
  for(int line = 0 ; line < linecount ; ++line){
    char* next = strchr(walker, '\n');
    if(next){
      *next = '\0';
    }
    int w = ncstrwidth(walker, NULL, NULL);
    if(w > maxwidth){
      maxwidth = w;
    }
    if(next){
      *next = '\n';
      walker = next + 1;
    }else{
      break;
    }
  }
  if(maxwidth <= 0){
    free(dup);
    return NULL;
  }
  int cols = maxwidth;
  if(cols > parent_cols){
    cols = parent_cols;
  }
  int xpos = (parent_cols - cols) / 2;

  struct ncplane_options nopts = {
    .y = ncplane_dim_y(parent) - (linecount + 1),
    .x = xpos,
    .rows = linecount,
    .cols = cols,
    .name = "subt",
  };
  struct ncplane* n = ncplane_create(parent, &nopts);
  if(n == NULL){
    free(dup);
    return NULL;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0xff, 0xff, 0xff);
  ncchannels_set_fg_alpha(&channels, NCALPHA_OPAQUE);
  ncchannels_set_bg_rgb8(&channels, 0x20, 0x20, 0x20);
  ncchannels_set_bg_alpha(&channels, NCALPHA_BLEND);
  ncplane_set_base(n, " ", 0, channels);

  ncplane_set_fg_rgb8(n, 0xff, 0xff, 0xff);
  ncplane_set_fg_alpha(n, NCALPHA_OPAQUE);
  ncplane_set_bg_rgb8(n, 0x20, 0x20, 0x20);
  ncplane_set_bg_alpha(n, NCALPHA_BLEND);
  if(logged_flag && !*logged_flag){
    SUBLOG_DEBUG("rendering subtitle text (raw): \"%s\"", raw);
    SUBLOG_DEBUG("rendering subtitle text (trimmed): \"%s\"", trimmed);
    *logged_flag = true;
  }
  char* linewalker = trimmed;
  for(int line = 0 ; line < linecount ; ++line){
    char* next = strchr(linewalker, '\n');
    if(next){
      *next = '\0';
    }
    ncplane_putstr_yx(n, line, 0, linewalker);
    if(next){
      *next = '\n';
      linewalker = next + 1;
    }else{
      break;
    }
  }
  free(dup);
  return n;
}

static uint32_t palette[NCPALETTESIZE];

struct ncplane* ffmpeg_subtitle(ncplane* parent, const ncvisual* ncv){
  if(!ncv->details->subtitle_active || ncv->details->subtitle.num_rects == 0){
    return NULL;
  }
  double now = ffmpeg_get_video_position_seconds(ncv);
  if(now >= 0.0){
    if(now < ncv->details->subtitle_start_time){
      return NULL;
    }
    if(now > ncv->details->subtitle_end_time){
      SUBLOG_DEBUG("subtitle expired at %.3f (end %.3f)", now, ncv->details->subtitle_end_time);
      avsubtitle_free(&ncv->details->subtitle);
      ncv->details->subtitle_active = false;
      return NULL;
    }
  }
  for(unsigned i = 0 ; i < ncv->details->subtitle.num_rects ; ++i){
    // it is possible that there are more than one subtitle rects present,
    // but we only bother dealing with the first one we find FIXME?
    const AVSubtitleRect* rect = ncv->details->subtitle.rects[i];
    if(rect->type == SUBTITLE_ASS){
      const char* render_text = NULL;
      render_text = rect->ass ? rect->ass : rect->text;
      if(!render_text){
        SUBLOG_DEBUG("ASS subtitle missing text, skipping");
        return NULL;
      }
      struct ncplane* n = subtitle_plane_from_text(parent, render_text, &ncv->details->subtitle_logged);
      if(!n){
        SUBLOG_WARN("failed to render ASS subtitle: \"%s\"", render_text);
      }
      return n;
    }else if(rect->type == SUBTITLE_TEXT){
      return subtitle_plane_from_text(parent, rect->text, &ncv->details->subtitle_logged);
    }else if(rect->type == SUBTITLE_BITMAP){
      // there are technically up to AV_NUM_DATA_POINTERS planes, but we
      // only try to work with the first FIXME?
      if(rect->linesize[0] != rect->w){
        SUBLOG_WARN("bitmap data linesize %d != width %d", rect->linesize[0], rect->w);
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
        SUBLOG_WARN("failed to construct ncvisual for bitmap subtitle");
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
        SUBLOG_WARN("failed to allocate ncplane for bitmap subtitle");
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
        SUBLOG_WARN("failed to blit bitmap subtitle rect");
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
          int result = 0;
          AVSubtitle decoded = {0};
          int ret = avcodec_decode_subtitle2(n->details->subtcodecctx, &decoded, &result, n->details->packet);
          if(ret >= 0 && result){
            SUBLOG_DEBUG("packet pts=%" PRId64 " produced %u rects (format %d)",
                         n->details->packet->pts, decoded.num_rects,
                         decoded.format);
            avsubtitle_free(&n->details->subtitle);
            n->details->subtitle = decoded;
            if(decoded.num_rects > 0){
              double base_time = ffmpeg_get_video_position_seconds(n);
              int64_t subtitle_pts = decoded.pts;
              if(subtitle_pts == AV_NOPTS_VALUE){
                subtitle_pts = n->details->packet->pts;
              }
              if(subtitle_pts != AV_NOPTS_VALUE && n->details->subtitle_time_base > 0.0){
                base_time = subtitle_pts * n->details->subtitle_time_base;
              }
              if(base_time < 0.0){
                base_time = 0.0;
              }
              const double start_offset = decoded.start_display_time / 1000.0;
              double end_offset = decoded.end_display_time / 1000.0;
              double start_time = base_time + start_offset;
              double end_time = (end_offset > 0.0) ? base_time + end_offset
                                                   : start_time + 5.0;
              if(end_time <= start_time){
                end_time = start_time + 5.0;
              }
              n->details->subtitle_start_time = start_time;
              n->details->subtitle_end_time = end_time;
              n->details->subtitle_active = true;
              n->details->subtitle_logged = false;
              SUBLOG_DEBUG("subtitle scheduled start=%.3f end=%.3f pts=%" PRId64
                           " start_ms=%u end_ms=%u base=%.3f tb=%.6f",
                           start_time, end_time, subtitle_pts,
                           decoded.start_display_time, decoded.end_display_time,
                           base_time, n->details->subtitle_time_base);
            }else{
              n->details->subtitle_active = false;
            }
          }else{
            if(ret < 0){
              SUBLOG_WARN("decode failure (%d) on pts=%" PRId64, ret, n->details->packet->pts);
            }
            avsubtitle_free(&decoded);
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
            }else if(audio_ret == AVERROR(EAGAIN)){
              // Decoder full - save this packet for next time
              if(audio_queue_enqueue(&n->details->pending_audio_packets, n->details->packet) == 0){
                n->details->audio_packet_outstanding = true;
              }
            }
            av_packet_unref(n->details->packet);

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
  int64_t vpts = f->pts;
  if(vpts == AV_NOPTS_VALUE){
    vpts = f->best_effort_timestamp;
  }
  n->details->last_video_frame_pts = vpts;
  if(n->details->decoded_frames >= 0){
    ++n->details->decoded_frames;
  }
  return 0;
}

static int64_t
ffmpeg_frame_index(const ncvisual* ncv){
  if(!ncv || !ncv->details){
    return -1;
  }
  return ncv->details->decoded_frames;
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
    deets->last_video_frame_pts = AV_NOPTS_VALUE;
    deets->decoded_frames = 0;
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
    deets->subtitle_active = false;
    deets->subtitle_start_time = 0.0;
    deets->subtitle_end_time = 0.0;
    deets->subtitle_time_base = 0.0;
    deets->subtitle_text_field = 9;
    deets->subtitle_logged = false;
    deets->subtitle_last_hash = 0;
    deets->subtitle_cached_text = NULL;
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
    AVStream* sst = ncv->details->fmtctx->streams[ncv->details->sub_stream_index];
    ncv->details->subtitle_time_base = av_q2d(sst->time_base);
    if(ncv->details->subtitle_time_base <= 0.0){
      ncv->details->subtitle_time_base = 1.0 / AV_TIME_BASE;
    }
    if(ncv->details->subtcodecctx->subtitle_header &&
       ncv->details->subtcodecctx->subtitle_header_size > 0){
      ncv->details->subtitle_text_field =
        ass_detect_text_field(ncv->details->subtcodecctx->subtitle_header,
                              ncv->details->subtcodecctx->subtitle_header_size);
      if(ncv->details->subtitle_text_field < 1){
        ncv->details->subtitle_text_field = 9;
      }
    }
    if(avcodec_parameters_to_context(ncv->details->subtcodecctx, sst->codecpar) < 0){
      goto err;
    }
    if(avcodec_open2(ncv->details->subtcodecctx, ncv->details->subtcodec, NULL) < 0){
      //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
      goto err;
    }
    const char* subtitle_codec_name = "unknown";
    int subtitle_codec_id = -1;
    if(ncv->details->subtcodec){
      subtitle_codec_id = (int)ncv->details->subtcodec->id;
      if(ncv->details->subtcodec->name){
        subtitle_codec_name = ncv->details->subtcodec->name;
      }
    }
    SUBLOG_INFO("stream %d (%s) discovered with codec id %d",
                ncv->details->sub_stream_index,
                subtitle_codec_name, subtitle_codec_id);
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
  int vthreads = ffmpeg_detect_thread_count();
  if(vthreads > 1){
    ncv->details->codecctx->thread_count = vthreads;
    if(ncv->details->codec->capabilities & AV_CODEC_CAP_FRAME_THREADS){
      ncv->details->codecctx->thread_type = FF_THREAD_FRAME;
    }else{
      ncv->details->codecctx->thread_type = FF_THREAD_SLICE;
    }
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
  free(deets->subtitle_cached_text);
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

// Public API functions used by ncplayer
API double
ffmpeg_get_video_position_seconds(const ncvisual* ncv){
  if(!ncv || !ncv->details || !ncv->details->fmtctx){
    return -1.0;
  }
  int stream_index = ncv->details->stream_index;
  if(stream_index < 0 || stream_index >= (int)ncv->details->fmtctx->nb_streams){
    return -1.0;
  }
  AVStream* stream = ncv->details->fmtctx->streams[stream_index];
  if(!stream){
    return -1.0;
  }
  double time_base = av_q2d(stream->time_base);
  if(time_base <= 0.0){
    return -1.0;
  }
  int64_t pts = ncv->details->last_video_frame_pts;
  if(pts == AV_NOPTS_VALUE){
    return -1.0;
  }
  return pts * time_base;
}

// Public API functions for audio handling (called from play.cpp)
// Define API macro for visibility export

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

    // Check if this is a new frame (different PTS) or the same one we already processed
    int64_t current_pts = ncv->details->audio_frame->pts;
    if(current_pts == ncv->details->last_audio_frame_pts && current_pts != AV_NOPTS_VALUE){
      // Same frame as before - don't process again
      // This can happen if avcodec_receive_frame is called multiple times
      // Note: avcodec_receive_frame should only return each frame once, so this
      // check is defensive. If we see this frequently, there's a bug elsewhere.
      return 0;
    }
    ncv->details->last_audio_frame_pts = current_pts;
    return ncv->details->audio_frame->nb_samples;
  }else if(averr == AVERROR(EAGAIN)){
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    // Need more packets - video decoder will provide them
    // This is normal - the decoder needs more packets before it can produce a frame
    (void)outstanding;
    return 0;
  }else if(averr == AVERROR_EOF){
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    return 1; // EOF
  }else{
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
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

API int
ffmpeg_seek_relative(ncvisual* ncv, double seconds){
  if(!ncv || !ncv->details || !ncv->details->fmtctx || seconds == 0.0){
    return 0;
  }
  if(!ncv->details->codecctx){
    return -1;
  }
  AVFormatContext* fmt = ncv->details->fmtctx;
  int stream_index = ncv->details->stream_index;
  if(stream_index < 0 || stream_index >= (int)fmt->nb_streams){
    return -1;
  }
  AVStream* stream = fmt->streams[stream_index];
  double time_base = av_q2d(stream->time_base);
  if(time_base == 0.0){
    return -1;
  }
  int64_t current = ncv->details->last_video_frame_pts;
  if(current == AV_NOPTS_VALUE){
    current = (stream->start_time != AV_NOPTS_VALUE) ? stream->start_time : 0;
  }
  int64_t offset = seconds / time_base;
  int64_t target = current + offset;
  if(target < 0){
    target = 0;
  }
  int flags = 0;
  if(seconds < 0){
    flags |= AVSEEK_FLAG_BACKWARD;
  }
  if(av_seek_frame(fmt, stream_index, target, flags) < 0){
    // clamp to valid duration if available
    if(stream->duration > 0 && target > stream->duration){
      target = stream->duration;
      if(av_seek_frame(fmt, stream_index, target, AVSEEK_FLAG_BACKWARD) < 0){
        return -1;
      }
    }else{
      return -1;
    }
  }
  avcodec_flush_buffers(ncv->details->codecctx);
  ncv->details->packet_outstanding = false;
  av_packet_unref(ncv->details->packet);
  ncv->details->last_video_frame_pts = target;
  double frame_rate = av_q2d(stream->avg_frame_rate);
  if(frame_rate <= 0.0){
    frame_rate = 1.0 / av_q2d(stream->time_base);
    if(frame_rate <= 0.0){
      frame_rate = 30.0;
    }
  }
  double seconds_pos = target * time_base;
  if(seconds_pos < 0){
    seconds_pos = 0;
  }
  ncv->details->decoded_frames = (int64_t)(seconds_pos * frame_rate);
  if(ncv->details->audiocodecctx){
    avcodec_flush_buffers(ncv->details->audiocodecctx);
    pthread_mutex_lock(&ncv->details->audio_packet_mutex);
    audio_queue_clear(&ncv->details->pending_audio_packets);
    ncv->details->audio_packet_outstanding = false;
    pthread_mutex_unlock(&ncv->details->audio_packet_mutex);
    ncv->details->last_audio_frame_pts = AV_NOPTS_VALUE;
  }
  if(ffmpeg_decode(ncv) < 0){
    return -1;
  }
  return 0;
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
  .visual_seek = ffmpeg_seek_relative,
  .visual_frame_index = ffmpeg_frame_index,
  .visual_destroy = ffmpeg_destroy,
  .rowalign = 64, // ffmpeg wants multiples of IMGALIGN (64)
  .canopen_images = true,
  .canopen_videos = true,
};

#endif
