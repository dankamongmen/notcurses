#include <string.h>
#include "version.h"
#include "internal.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;

typedef struct ncvisual {
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;       // video codec context
  struct AVCodecContext* subtcodecctx;   // subtitle codec context
  struct AVFrame* frame;
  struct AVFrame* oframe;
  struct AVCodec* codec;
  struct AVCodecParameters* cparams;
  struct AVCodec* subtcodec;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  int packet_outstanding;
  int dstwidth, dstheight;
  int stream_index;        // match against this following av_read_frame()
  int sub_stream_index;    // subtitle stream index, can be < 0 if no subtitles
  float timescale;         // scale frame duration by this value
  ncplane* ncp;
  // if we're creating the plane based off the first frame's dimensions, these
  // describe where the plane ought be placed, and how it ought be sized. this
  // path sets ncobj. ncvisual_destroy() ought in that case kill the ncplane.
  int placex, placey;
  ncscale_e style;         // none, scale, or stretch
  struct notcurses* ncobj; // set iff this ncvisual "owns" its ncplane
#ifdef USE_FFMPEG
  AVSubtitle subtitle;
#endif
} ncvisual;

#ifdef USE_FFMPEG
ncplane* ncvisual_plane(ncvisual* ncv){
  return ncv->ncp;
}

void ncvisual_destroy(ncvisual* ncv){
  if(ncv){
    avcodec_close(ncv->codecctx);
    avcodec_free_context(&ncv->codecctx);
    av_frame_free(&ncv->frame);
    av_freep(&ncv->oframe);
    //avcodec_parameters_free(&ncv->cparams);
    sws_freeContext(ncv->swsctx);
    av_packet_free(&ncv->packet);
    avformat_close_input(&ncv->fmtctx);
    avsubtitle_free(&ncv->subtitle);
    if(ncv->ncobj && ncv->ncp){
      ncplane_destroy(ncv->ncp);
    }
    free(ncv);
  }
}

bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return true;
}

static ncvisual*
ncvisual_create(float timescale){
  ncvisual* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return NULL;
  }
  memset(ret, 0, sizeof(*ret));
  ret->timescale = timescale;
  return ret;
}

/* static void
print_frame_summary(const AVCodecContext* cctx, const AVFrame* f){
  char pfmt[128];
  av_get_pix_fmt_string(pfmt, sizeof(pfmt), f->format);
  fprintf(stderr, "Frame %05d (%d? %d?) %dx%d pfmt %d (%s)\n",
          cctx->frame_number,
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

char* ncvisual_subtitle(const ncvisual* ncv){
  for(unsigned i = 0 ; i < ncv->subtitle.num_rects ; ++i){
    const AVSubtitleRect* rect = ncv->subtitle.rects[i];
    if(rect->type == SUBTITLE_ASS){
      return deass(rect->ass);
    }else if(rect->type == SUBTITLE_TEXT) {;
      return strdup(rect->text);
    }
  }
  return NULL;
}

static nc_err_e
averr2ncerr(int averr){
  // FIXME need to map averror codes to ncerrors
//fprintf(stderr, "AVERR: %d/%x %d/%x\n", averr, averr, -averr, -averr);
  return -averr;
}

nc_err_e ncvisual_decode(ncvisual* nc){
  bool have_frame = false;
  bool unref = false;
  av_freep(&nc->oframe->data[0]);
  do{
    do{
      if(nc->packet_outstanding){
        break;
      }
      if(unref){
        av_packet_unref(nc->packet);
      }
      int averr;
      if((averr = av_read_frame(nc->fmtctx, nc->packet)) < 0){
        /*if(averr != AVERROR_EOF){
          fprintf(stderr, "Error reading frame info (%s)\n", av_err2str(*averr));
        }*/
        return averr2ncerr(averr);
      }
      unref = true;
      if(nc->packet->stream_index == nc->sub_stream_index){
        int result = 0, ret;
        ret = avcodec_decode_subtitle2(nc->subtcodecctx, &nc->subtitle, &result, nc->packet);
        if(ret >= 0 && result){
          // FIXME?
        }
      }
    }while(nc->packet->stream_index != nc->stream_index);
    ++nc->packet_outstanding;
    if(avcodec_send_packet(nc->codecctx, nc->packet) < 0){
      //fprintf(stderr, "Error processing AVPacket (%s)\n", av_err2str(*ncerr));
      return ncvisual_decode(nc);
    }
    --nc->packet_outstanding;
    av_packet_unref(nc->packet);
    int averr = avcodec_receive_frame(nc->codecctx, nc->frame);
    if(averr >= 0){
      have_frame = true;
    }else if(averr == AVERROR(EAGAIN) || averr == AVERROR_EOF){
      have_frame = false;
    }else if(averr < 0){
      //fprintf(stderr, "Error decoding AVPacket (%s)\n", av_err2str(averr));
      return averr2ncerr(averr);
    }
  }while(!have_frame);
//print_frame_summary(nc->codecctx, nc->frame);
#define IMGALLOCALIGN 32
  int rows, cols;
  if(nc->ncp == NULL){ // create plane
    if(nc->style == NCSCALE_NONE){
      rows = nc->frame->height / 2;
      cols = nc->frame->width;
    }else{ // FIXME differentiate between scale/stretch
      notcurses_term_dim_yx(nc->ncobj, &rows, &cols);
      if(nc->placey >= rows || nc->placex >= cols){
        return NCERR_DECODE;
      }
      rows -= nc->placey;
      cols -= nc->placex;
    }
    nc->dstwidth = cols;
    nc->dstheight = rows * 2;
    nc->ncp = ncplane_new(nc->ncobj, rows, cols, nc->placey, nc->placex, NULL);
    nc->placey = 0;
    nc->placex = 0;
    if(nc->ncp == NULL){
      return NCERR_NOMEM;
    }
  }else{ // check for resize
    ncplane_dim_yx(nc->ncp, &rows, &cols);
    if(rows != nc->dstheight / 2 || cols != nc->dstwidth){
      sws_freeContext(nc->swsctx);
      nc->swsctx = NULL;
      nc->dstheight = rows * 2;
      nc->dstwidth = cols;
    }
  }
  const int targformat = AV_PIX_FMT_RGBA;
  nc->swsctx = sws_getCachedContext(nc->swsctx,
                                    nc->frame->width,
                                    nc->frame->height,
                                    nc->frame->format,
                                    nc->dstwidth,
                                    nc->dstheight,
                                    targformat,
                                    SWS_LANCZOS,
                                    NULL, NULL, NULL);
  if(nc->swsctx == NULL){
    //fprintf(stderr, "Error retrieving swsctx\n");
    return NCERR_DECODE;
  }
  memcpy(nc->oframe, nc->frame, sizeof(*nc->oframe));
  nc->oframe->format = targformat;
  nc->oframe->width = nc->dstwidth;
  nc->oframe->height = nc->dstheight;
  int size = av_image_alloc(nc->oframe->data, nc->oframe->linesize,
                            nc->oframe->width, nc->oframe->height,
                            nc->oframe->format, IMGALLOCALIGN);
  if(size < 0){
    //fprintf(stderr, "Error allocating visual data (%s)\n", av_err2str(size));
    return NCERR_NOMEM;
  }
  int height = sws_scale(nc->swsctx, (const uint8_t* const*)nc->frame->data,
                         nc->frame->linesize, 0,
                         nc->frame->height, nc->oframe->data, nc->oframe->linesize);
  if(height < 0){
    //fprintf(stderr, "Error applying scaling (%s)\n", av_err2str(height));
    return NCERR_NOMEM;
  }
//print_frame_summary(nc->codecctx, nc->oframe);
#undef IMGALLOCALIGN
  av_frame_unref(nc->frame);
  return NCERR_SUCCESS;
}

static ncvisual*
ncvisual_open(const char* filename, nc_err_e* ncerr){
  ncvisual* ncv = ncvisual_create(1);
  if(ncv == NULL){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *ncerr = NCERR_NOMEM;
    return NULL;
  }
  memset(ncv, 0, sizeof(*ncv));
  int averr = avformat_open_input(&ncv->fmtctx, filename, NULL, NULL);
  if(averr < 0){
    // fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(*averr));
    *ncerr = averr2ncerr(averr);
    goto err;
  }
  averr = avformat_find_stream_info(ncv->fmtctx, NULL);
  if(averr < 0){
    /*fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(*averr));*/
    *ncerr = averr2ncerr(averr);
    goto err;
  }
//av_dump_format(ncv->fmtctx, 0, filename, false);
  if((averr = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, &ncv->subtcodec, 0)) >= 0){
    ncv->sub_stream_index = averr;
    if((ncv->subtcodecctx = avcodec_alloc_context3(ncv->subtcodec)) == NULL){
      //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
      *ncerr = NCERR_NOMEM;
      goto err;
    }
    // FIXME do we need avcodec_parameters_to_context() here?
    if((averr = avcodec_open2(ncv->subtcodecctx, ncv->subtcodec, NULL)) < 0){
      //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
      *ncerr = averr2ncerr(averr);
      goto err;
    }
  }else{
    ncv->sub_stream_index = -1;
  }
  if((ncv->packet = av_packet_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    *ncerr = NCERR_NOMEM;
    goto err;
  }
  if((averr = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->codec, 0)) < 0){
    // fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    *ncerr = averr2ncerr(averr);
    goto err;
  }
  ncv->stream_index = averr;
  if(ncv->codec == NULL){
    //fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  AVStream* st = ncv->fmtctx->streams[ncv->stream_index];
  if((ncv->codecctx = avcodec_alloc_context3(ncv->codec)) == NULL){
    //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    *ncerr = NCERR_NOMEM;
    goto err;
  }
  if(avcodec_parameters_to_context(ncv->codecctx, st->codecpar) < 0){
    goto err;
  }
  if((averr = avcodec_open2(ncv->codecctx, ncv->codec, NULL)) < 0){
    //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
    *ncerr = averr2ncerr(averr);
    goto err;
  }
  /*if((ncv->cparams = avcodec_parameters_alloc()) == NULL){
    //fprintf(stderr, "Couldn't allocate codec params for %s\n", filename);
    *averr = NCERR_NOMEM;
    goto err;
  }
  if((*averr = avcodec_parameters_from_context(ncv->cparams, ncv->codecctx)) < 0){
    //fprintf(stderr, "Couldn't get codec params for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }*/
  if((ncv->frame = av_frame_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate frame for %s\n", filename);
    *ncerr = NCERR_NOMEM;
    goto err;
  }
  if((ncv->oframe = av_frame_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate output frame for %s\n", filename);
    *ncerr = NCERR_NOMEM;
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return NULL;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, nc_err_e* ncerr){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == NULL){
    return NULL;
  }
  ncplane_dim_yx(nc, &ncv->dstheight, &ncv->dstwidth);
  ncv->dstheight *= 2;
  ncv->ncp = nc;
  ncv->style = NCSCALE_STRETCH;
  return ncv;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              nc_err_e* ncerr, int y, int x, ncscale_e style){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == NULL){
    return NULL;
  }
  ncv->placey = y;
  ncv->placex = x;
  ncv->style = style;
  ncv->ncobj = nc;
  ncv->ncp = NULL;
  return ncv;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
//fprintf(stderr, "render %dx%d+%dx%d\n", begy, begx, leny, lenx);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return -1;
  }
  const AVFrame* f = ncv->oframe;
  if(f == NULL){
    return -1;
  }
//fprintf(stderr, "render %d/%d to %dx%d+%dx%d\n", f->height, f->width, begy, begx, leny, lenx);
  if(begx >= f->width || begy >= f->height){
    return -1;
  }
  if(lenx == -1){ // -1 means "to the end"; use all space available
    lenx = f->width - begx;
  }
  if(leny == -1){
    leny = f->height - begy;
  }
  if(lenx == 0 || leny == 0){ // no need to draw zero-size object, exit
    return 0;
  }
  if(begx + lenx > f->width || begy + leny > f->height){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(ncv->ncp, &dimy, &dimx);
  ncplane_cursor_move_yx(ncv->ncp, 0, 0);
  const int linesize = f->linesize[0];
  void* data = f->data[0];
  // y and x are actual plane coordinates. each row corresponds to two rows of
  // the input (scaled) frame (columns are 1:1). we track the row of the
  // visual via visy.
//fprintf(stderr, "render: %dx%d:%d+%d of %d/%d -> %dx%d\n", begy, begx, leny, lenx, f->height, f->width, dimy, dimx);
  int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(f->format));
  if(bpp != 32){
	  return -1;
  }
  int ret = ncblit_rgba(ncv->ncp, ncv->placey, ncv->placex, linesize, data,
                        begy, begx, leny, lenx);
  //av_frame_unref(ncv->oframe);
  return ret;
}

// iterative over the decoded frames, calling streamer() with curry for each.
// frames carry a presentation time relative to the beginning, so we get an
// initial timestamp, and check each frame against the elapsed time to sync
// up playback.
int ncvisual_stream(notcurses* nc, ncvisual* ncv, nc_err_e* ncerr,
                    float timescale, streamcb streamer, void* curry){
  int frame = 1;
  ncv->timescale = timescale;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  uint64_t nsbegin = timespec_to_ns(&begin);
  bool usets = false;
  // each frame has a pkt_duration in milliseconds. keep the aggregate, in case
  // we don't have PTS available.
  uint64_t sum_duration = 0;
  while((*ncerr = ncvisual_decode(ncv)) == NCERR_SUCCESS){
    // codecctx seems to be off by a factor of 2 regularly. instead, go with
    // the time_base from the avformatctx.
    double tbase = av_q2d(ncv->fmtctx->streams[ncv->stream_index]->time_base);
    int64_t ts = ncv->oframe->best_effort_timestamp;
    if(frame == 1 && ts){
      usets = true;
    }
    if(ncvisual_render(ncv, 0, 0, -1, -1) < 0){
      return -1;
    }
    if(streamer){
      int r = streamer(nc, ncv, curry);
      if(r){
        return r;
      }
    }
    ++frame;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nsnow = timespec_to_ns(&now);
    struct timespec interval;
    uint64_t duration = ncv->oframe->pkt_duration * tbase * NANOSECS_IN_SEC;
//fprintf(stderr, "use: %u dur: %ju ts: %ju cctx: %f fctx: %f\n", usets, duration, ts, av_q2d(ncv->codecctx->time_base), av_q2d(ncv->fmtctx->streams[ncv->stream_index]->time_base));
    sum_duration += (duration * ncv->timescale);
    double schedns = nsbegin;
    if(usets){
      if(tbase == 0){
        tbase = duration;
      }
      schedns += ts * (tbase * ncv->timescale) * NANOSECS_IN_SEC;
    }else{
      schedns += sum_duration;
    }
    if(nsnow < schedns){
      ns_to_timespec(schedns - nsnow, &interval);
      nanosleep(&interval, NULL);
    }
  }
  if(*ncerr == NCERR_EOF){
    return 0;
  }
  return -1;
}

int ncvisual_init(int loglevel){
  av_log_set_level(loglevel);
  // FIXME could also use av_log_set_callback() and capture the message...
  return 0;
}
#else // built without ffmpeg
#ifndef USE_OIIO // built without ffmpeg or oiio
bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return false;
}

ncplane* ncvisual_plane(ncvisual* ncv){
  return ncv->ncp;
}

nc_err_e ncvisual_decode(ncvisual* nc){
  (void)nc;
  return NCERR_UNIMPLEMENTED;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
  (void)ncv;
  (void)begy;
  (void)begx;
  (void)leny;
  (void)lenx;
  return -1;
}

int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, nc_err_e* ncerr,
                    float timespec, streamcb streamer, void* curry){
  (void)nc;
  (void)ncv;
  (void)ncerr;
  (void)timespec;
  (void)streamer;
  (void)curry;
  return -1;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, nc_err_e* ncerr){
  (void)nc;
  (void)filename;
  (void)ncerr;
  return NULL;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              nc_err_e* ncerr, int y, int x, ncscale_e style){
  (void)nc;
  (void)filename;
  (void)ncerr;
  (void)y;
  (void)x;
  (void)style;
  return NULL;
}

char* ncvisual_subtitle(const ncvisual* ncv){
  (void)ncv;
  return NULL;
}

int ncvisual_init(int loglevel){
  (void)loglevel;
  return 0; // allow success here
}

void ncvisual_destroy(ncvisual* ncv){
  assert(!ncv);
  (void)ncv;
}

#endif
#endif
