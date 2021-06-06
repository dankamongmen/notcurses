#include "builddef.h"
#ifdef USE_FFMPEG
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include <libswscale/version.h>
#include <libavformat/version.h>
#include <libavformat/avformat.h>
#include "internal.h"
#include "visual-details.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;

typedef struct ncvisual_details {
  int packet_outstanding;
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;     // video codec context
  struct AVCodecContext* subtcodecctx; // subtitle codec context
  struct AVFrame* frame;               // frame as read
  struct AVCodec* codec;
  struct AVCodecParameters* cparams;
  struct AVCodec* subtcodec;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  struct SwsContext* rgbactx;
  AVSubtitle subtitle;
  int stream_index;        // match against this following av_read_frame()
  int sub_stream_index;    // subtitle stream index, can be < 0 if no subtitles
} ncvisual_details;

#define IMGALLOCALIGN 64

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
  fprintf(stderr, " PTS %ld Flags: 0x%04x\n", f->pts, f->flags);
  fprintf(stderr, " %lums@%lums (%skeyframe) qual: %d\n",
          f->pkt_duration, // FIXME in 'time_base' units
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

char* ffmpeg_subtitle(const ncvisual* ncv){
  for(unsigned i = 0 ; i < ncv->details->subtitle.num_rects ; ++i){
    const AVSubtitleRect* rect = ncv->details->subtitle.rects[i];
    if(rect->type == SUBTITLE_ASS){
      return deass(rect->ass);
    }else if(rect->type == SUBTITLE_TEXT){;
      return strdup(rect->text);
    }else if(rect->type == SUBTITLE_BITMAP){
      // FIXME
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
    av_freep(&sframe->data[0]);
    av_freep(&sframe);
    return -1;
  }
  int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(sframe->format));
  if(bpp != 32){
//fprintf(stderr, "Bad bits-per-pixel (wanted 32, got %d)\n", bpp);
    av_freep(&sframe->data[0]);
    av_freep(&sframe);
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

int ffmpeg_decode(ncvisual* n){
  if(n->details->fmtctx == NULL){ // not a file-backed ncvisual
    return -1;
  }
  bool have_frame = false;
  bool unref = false;
  do{
    do{
      if(n->details->packet_outstanding){
        break;
      }
      if(unref){
        av_packet_unref(n->details->packet);
      }
      int averr;
      if((averr = av_read_frame(n->details->fmtctx, n->details->packet)) < 0){
        /*if(averr != AVERROR_EOF){
          fprintf(stderr, "Error reading frame info (%s)\n", av_err2str(averr));
        }*/
        return averr2ncerr(averr);
      }
      unref = true;
      if(n->details->packet->stream_index == n->details->sub_stream_index){
        int result = 0, ret;
        ret = avcodec_decode_subtitle2(n->details->subtcodecctx, &n->details->subtitle, &result, n->details->packet);
        if(ret >= 0 && result){
          // FIXME?
        }
      }
    }while(n->details->packet->stream_index != n->details->stream_index);
    ++n->details->packet_outstanding;
    int averr = avcodec_send_packet(n->details->codecctx, n->details->packet);
    if(averr < 0){
//fprintf(stderr, "Error processing AVPacket\n");
      return averr2ncerr(averr);
    }
    --n->details->packet_outstanding;
    av_packet_unref(n->details->packet);
    averr = avcodec_receive_frame(n->details->codecctx, n->details->frame);
    if(averr >= 0){
      have_frame = true;
    }else if(averr == AVERROR(EAGAIN) || averr == AVERROR_EOF){
      have_frame = false;
    }else if(averr < 0){
//fprintf(stderr, "Error decoding AVPacket\n");
      return averr2ncerr(averr);
    }
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

ncvisual_details* ffmpeg_details_init(void){
  ncvisual_details* deets = malloc(sizeof(*deets));
  if(deets){
    memset(deets, 0, sizeof(*deets));
    deets->stream_index = -1;
    deets->sub_stream_index = -1;
    if((deets->frame = av_frame_alloc()) == NULL){
      free(deets);
      return NULL;
    }
  }
  return deets;
}

ncvisual* ffmpeg_create(){
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

ncvisual* ffmpeg_from_file(const char* filename){
  AVStream* st;
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
  if((averr = av_find_best_stream(ncv->details->fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, &ncv->details->subtcodec, 0)) >= 0){
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
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details->frame);
  if((ncv->details->packet = av_packet_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    goto err;
  }
  if((averr = av_find_best_stream(ncv->details->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->details->codec, 0)) < 0){
    // fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  ncv->details->stream_index = averr;
  if(ncv->details->codec == NULL){
    //fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  st = ncv->details->fmtctx->streams[ncv->details->stream_index];
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
  /*if((ncv->cparams = avcodec_parameters_alloc()) == NULL){
    //fprintf(stderr, "Couldn't allocate codec params for %s\n", filename);
    goto err;
  }
  if((*averr = avcodec_parameters_from_context(ncv->cparams, ncv->details->codecctx)) < 0){
    //fprintf(stderr, "Couldn't get codec params for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }*/
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

// iterate over the decoded frames, calling streamer() with curry for each.
// frames carry a presentation time relative to the beginning, so we get an
// initial timestamp, and check each frame against the elapsed time to sync
// up playback.
int ffmpeg_stream(notcurses* nc, ncvisual* ncv, float timescale,
                  ncstreamcb streamer, const struct ncvisual_options* vopts,
                  void* curry){
  int frame = 1;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  uint64_t nsbegin = timespec_to_ns(&begin);
  //bool usets = false;
  // each frame has a pkt_duration in milliseconds. keep the aggregate, in case
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
    if(activevopts.n){
      ncplane_erase(activevopts.n); // new frame could be partially transparent
    }
    // decay the blitter explicitly, so that the callback knows the blitter it
    // was actually rendered with
    ncvisual_blitter_geom(nc, ncv, &activevopts, NULL, NULL, NULL, NULL,
                          &activevopts.blitter);
    if((newn = ncvisual_render(nc, ncv, &activevopts)) == NULL){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return -1;
    }
    if(activevopts.n != newn){
      activevopts.n = newn;
    }
    ++frame;
    uint64_t duration = ncv->details->frame->pkt_duration * tbase * NANOSECS_IN_SEC;
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

int ffmpeg_decode_loop(ncvisual* ncv){
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

uint32_t* ffmpeg_resize_internal(const ncvisual* ncv, int rows, int* stride,
                                 int cols, const blitterargs* bargs){
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
  int height = sws_scale(ncv->details->swsctx, (const uint8_t* const*)inframe->data,
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
int ffmpeg_resize(ncvisual* n, int rows, int cols){
  struct blitterargs bargs = {};
  int stride;
  void* data = ffmpeg_resize_internal(n, rows, &stride, cols, &bargs);
  if(data == n->data){ // no change, return
    return 0;
  }
  AVFrame* sframe = av_frame_alloc();
  if(sframe == NULL){
//fprintf(stderr, "Couldn't allocate output frame for scaled frame\n");
    return -1;
  }
  const AVFrame* inf = n->details->frame;
//fprintf(stderr, "WHN NCV: %d/%d\n", inf->width, inf->height);
  memcpy(sframe, inf, sizeof(*inf));
  sframe->format = AV_PIX_FMT_RGBA;
  sframe->width = cols;
  sframe->height = rows;
  memset(sframe->data, 0, sizeof(sframe->data));
  sframe->data[0] = data;
  memset(sframe->linesize, 0, sizeof(sframe->linesize));
  sframe->linesize[0] = stride;
  n->rowstride = sframe->linesize[0];
  n->pixy = rows;
  n->pixx = cols;
  if(n->owndata){
    av_frame_unref(n->details->frame);
  }
  ncvisual_set_data(n, sframe->data[0], true);
  n->details->frame = sframe;
//fprintf(stderr, "SIZE SCALED: %d %d (%u)\n", n->details->frame->height, n->details->frame->width, n->details->frame->linesize[0]);
  return 0;
}

// rows/cols: scaled output geometry (pixels)
int ffmpeg_blit(ncvisual* ncv, int rows, int cols, ncplane* n,
                const struct blitset* bset, const blitterargs* bargs){
  void* data;
  int stride = 0;
  data = ffmpeg_resize_internal(ncv, rows, &stride, cols, bargs);
  if(data == NULL){
    return -1;
  }
//fprintf(stderr, "WHN NCV: %d/%d bargslen: %d/%d targ: %d/%d\n", inframe->width, inframe->height, bargs->leny, bargs->lenx, rows, cols);
  int ret = 0;
  if(rgba_blit_dispatch(n, bset, stride, data, rows, cols, bargs) < 0){
//fprintf(stderr, "rgba dispatch failed!\n");
    ret = -1;
  }
  if(data != ncv->data){
    av_freep(&data); // &dptrs[0]
  }
  return ret;
}

void ffmpeg_details_seed(ncvisual* ncv){
  ncv->details->frame->data[0] = (uint8_t*)ncv->data;
  ncv->details->frame->data[1] = NULL;
  ncv->details->frame->linesize[0] = ncv->rowstride;
  ncv->details->frame->linesize[1] = 0;
  ncv->details->frame->width = ncv->pixx;
  ncv->details->frame->height = ncv->pixy;
  ncv->details->frame->format = AV_PIX_FMT_RGBA;
}

int ffmpeg_log_level(int level){
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

int ffmpeg_init(int loglevel){
  av_log_set_level(ffmpeg_log_level(loglevel));
  // FIXME could also use av_log_set_callback() and capture the message...
  return 0;
}

void ffmpeg_printbanner(const notcurses* nc __attribute__ ((unused))){
  printf("  avformat %u.%u.%u avutil %u.%u.%u swscale %u.%u.%u\n",
         LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
         LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
         LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
}

void ffmpeg_details_destroy(ncvisual_details* deets){
  avcodec_close(deets->codecctx);
  avcodec_free_context(&deets->subtcodecctx);
  avcodec_free_context(&deets->codecctx);
  av_frame_free(&deets->frame);
  //avcodec_parameters_free(&ncv->cparams);
  sws_freeContext(deets->rgbactx);
  sws_freeContext(deets->swsctx);
  av_packet_free(&deets->packet);
  avformat_close_input(&deets->fmtctx);
  avsubtitle_free(&deets->subtitle);
  free(deets);
}

void ffmpeg_destroy(ncvisual* ncv){
  if(ncv){
    ffmpeg_details_destroy(ncv->details);
    if(ncv->owndata){
      free(ncv->data);
    }
    free(ncv);
  }
}

static const ncvisual_implementation ffmpeg_impl = {
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
  .canopen_images = true,
  .canopen_videos = true,
};

const ncvisual_implementation* local_visual_implementation = &ffmpeg_impl;

#endif
