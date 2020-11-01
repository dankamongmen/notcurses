#include "builddef.h"
#ifdef USE_FFMPEG
#include "ffmpeg.h"
#include "internal.h"
#include "visual-details.h"

#define IMGALLOCALIGN 32

bool notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

bool notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

/*static void
print_frame_summary(const AVCodecContext* cctx, const AVFrame* f) {
  char pfmt[128];
  av_get_pix_fmt_string(pfmt, sizeof(pfmt), static_cast<enum AVPixelFormat>(f->format));
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
deass(const char* ass) {
  // SSA/ASS formats:
  // Dialogue: Marked=0,0:02:40.65,0:02:41.79,Wolf main,Cher,0000,0000,0000,,Et les enregistrements de ses ondes delta ?
  // FIXME more
  if(strncmp(ass, "Dialogue:", strlen("Dialogue:"))){
    return nullptr;
  }
  const char* delim = strchr(ass, ',');
  int commas = 0; // we want 8
  while(delim && commas < 8){
    delim = strchr(delim + 1, ',');
    ++commas;
  }
  if(!delim){
    return nullptr;
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

auto ncvisual_subtitle(const ncvisual* ncv) -> char* {
  for(unsigned i = 0 ; i < ncv->details.subtitle.num_rects ; ++i){
    const AVSubtitleRect* rect = ncv->details.subtitle.rects[i];
    if(rect->type == SUBTITLE_ASS){
      return deass(rect->ass);
    }else if(rect->type == SUBTITLE_TEXT) {;
      return strdup(rect->text);
    }
  }
  return nullptr;
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

int ncvisual_decode(ncvisual* nc){
  if(nc->details.fmtctx == nullptr){ // not a file-backed ncvisual
    return -1;
  }
  bool have_frame = false;
  bool unref = false;
  // FIXME what if this was set up with e.g. ncvisual_from_rgba()?
  if(nc->details.oframe){
    av_freep(&nc->details.oframe->data[0]);
  }
  do{
    do{
      if(nc->details.packet_outstanding){
        break;
      }
      if(unref){
        av_packet_unref(nc->details.packet);
      }
      int averr;
      if((averr = av_read_frame(nc->details.fmtctx, nc->details.packet)) < 0){
        /*if(averr != AVERROR_EOF){
          fprintf(stderr, "Error reading frame info (%s)\n", av_err2str(*averr));
        }*/
        return averr2ncerr(averr);
      }
      unref = true;
      if(nc->details.packet->stream_index == nc->details.sub_stream_index){
        int result = 0, ret;
        ret = avcodec_decode_subtitle2(nc->details.subtcodecctx, &nc->details.subtitle, &result, nc->details.packet);
        if(ret >= 0 && result){
          // FIXME?
        }
      }
    }while(nc->details.packet->stream_index != nc->details.stream_index);
    ++nc->details.packet_outstanding;
    if(avcodec_send_packet(nc->details.codecctx, nc->details.packet) < 0){
      //fprintf(stderr, "Error processing AVPacket (%s)\n", av_err2str(*ncerr));
      return ncvisual_decode(nc);
    }
    --nc->details.packet_outstanding;
    av_packet_unref(nc->details.packet);
    int averr = avcodec_receive_frame(nc->details.codecctx, nc->details.frame);
    if(averr >= 0){
      have_frame = true;
    }else if(averr == AVERROR(EAGAIN) || averr == AVERROR_EOF){
      have_frame = false;
    }else if(averr < 0){
      //fprintf(stderr, "Error decoding AVPacket (%s)\n", av_err2str(averr));
      return averr2ncerr(averr);
    }
  }while(!have_frame);
//print_frame_summary(nc->details.codecctx, nc->details.frame);
  const AVFrame* f = nc->details.frame;
  nc->rowstride = f->linesize[0];
  nc->cols = nc->details.frame->width;
  nc->rows = nc->details.frame->height;
//fprintf(stderr, "good decode! %d/%d %d %p\n", nc->details.frame->height, nc->details.frame->width, nc->rowstride, f->data);
  ncvisual_set_data(nc, reinterpret_cast<uint32_t*>(f->data[0]), false);
  return 0;
}

// resize frame to oframe, converting to RGBA (if necessary) along the way
int ncvisual_resize(ncvisual* nc, int rows, int cols) {
  const int targformat = AV_PIX_FMT_RGBA;
  AVFrame* inf = nc->details.oframe ? nc->details.oframe : nc->details.frame;
//fprintf(stderr, "got format: %d (%d/%d) want format: %d (%d/%d)\n", inf->format, nc->rows, nc->cols, targformat, rows, cols);
  if(inf->format == targformat && nc->rows == rows && nc->cols == cols){
    return 0;
  }
  auto swsctx = sws_getContext(inf->width,
                               inf->height,
                               static_cast<AVPixelFormat>(inf->format),
                               cols, rows,
                               static_cast<AVPixelFormat>(targformat),
                               SWS_LANCZOS, nullptr, nullptr, nullptr);
  if(swsctx == nullptr){
    //fprintf(stderr, "Error retrieving swsctx\n");
    return -1;
  }
  AVFrame* sframe;
  if((sframe = av_frame_alloc()) == nullptr){
    // fprintf(stderr, "Couldn't allocate frame for %s\n", filename);
    sws_freeContext(swsctx);
    return -1; // no need to free swsctx
  }
  memcpy(sframe, inf, sizeof(*sframe));
  sframe->format = targformat;
  sframe->width = cols;
  sframe->height = rows;
//fprintf(stderr, "SIZE DECODED: %d %d (%d) (want %d %d)\n", nc->rows, nc->cols, inf->linesize[0], rows, cols);
  int size = av_image_alloc(sframe->data, sframe->linesize,
                            sframe->width, sframe->height,
                            static_cast<AVPixelFormat>(sframe->format),
                            IMGALLOCALIGN);
  if(size < 0){
//fprintf(stderr, "Error allocating visual data (%d)\n", size);
    av_freep(&sframe);
    sws_freeContext(swsctx);
    return -1;
  }
  int height = sws_scale(swsctx, inf->data,
                         inf->linesize, 0,
                         inf->height, sframe->data,
                         sframe->linesize);
  sws_freeContext(swsctx);
  if(height < 0){
    //fprintf(stderr, "Error applying scaling (%s)\n", av_err2str(height));
    av_freep(sframe->data);
    av_freep(&sframe);
    return -1;
  }
  const AVFrame* f = sframe;
  int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(static_cast<AVPixelFormat>(f->format)));
  if(bpp != 32){
//fprintf(stderr, "Bad bits-per-pixel (wanted 32, got %d)\n", bpp);
    av_freep(sframe->data);
    av_freep(&sframe);
    return -1;
  }
  nc->rowstride = sframe->linesize[0];
  nc->rows = rows;
  nc->cols = cols;
  ncvisual_set_data(nc, reinterpret_cast<uint32_t*>(sframe->data[0]), true);
  if(nc->details.oframe){
    //av_freep(nc->details.oframe->data);
    av_freep(&nc->details.oframe);
  }
  nc->details.oframe = sframe;

//fprintf(stderr, "SIZE SCALED: %d %d (%u)\n", nc->details.oframe->height, nc->details.oframe->width, nc->details.oframe->linesize[0]);
  return 0;
}

ncvisual* ncvisual_from_file(const char* filename) {
  AVStream* st;
  ncvisual* ncv = ncvisual_create();
  if(ncv == nullptr){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    return nullptr;
  }
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details.frame);
  int averr = avformat_open_input(&ncv->details.fmtctx, filename, nullptr, nullptr);
  if(averr < 0){
//fprintf(stderr, "Couldn't open %s (%d)\n", filename, averr);
    goto err;
  }
  averr = avformat_find_stream_info(ncv->details.fmtctx, nullptr);
  if(averr < 0){
//fprintf(stderr, "Error extracting stream info from %s (%d)\n", filename, averr);
    goto err;
  }
//av_dump_format(ncv->details.fmtctx, 0, filename, false);
  if((averr = av_find_best_stream(ncv->details.fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, &ncv->details.subtcodec, 0)) >= 0){
    ncv->details.sub_stream_index = averr;
    if((ncv->details.subtcodecctx = avcodec_alloc_context3(ncv->details.subtcodec)) == nullptr){
      //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
      goto err;
    }
    // FIXME do we need avcodec_parameters_to_context() here?
    if((averr = avcodec_open2(ncv->details.subtcodecctx, ncv->details.subtcodec, nullptr)) < 0){
      //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
      goto err;
    }
  }else{
    ncv->details.sub_stream_index = -1;
  }
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details.frame);
  if((ncv->details.packet = av_packet_alloc()) == nullptr){
    // fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    goto err;
  }
  if((averr = av_find_best_stream(ncv->details.fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->details.codec, 0)) < 0){
    // fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  ncv->details.stream_index = averr;
  if(ncv->details.codec == nullptr){
    //fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  st = ncv->details.fmtctx->streams[ncv->details.stream_index];
  if((ncv->details.codecctx = avcodec_alloc_context3(ncv->details.codec)) == nullptr){
    //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    goto err;
  }
  if(avcodec_parameters_to_context(ncv->details.codecctx, st->codecpar) < 0){
    goto err;
  }
  if((averr = avcodec_open2(ncv->details.codecctx, ncv->details.codec, nullptr)) < 0){
    //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  /*if((ncv->cparams = avcodec_parameters_alloc()) == nullptr){
    //fprintf(stderr, "Couldn't allocate codec params for %s\n", filename);
    goto err;
  }
  if((*averr = avcodec_parameters_from_context(ncv->cparams, ncv->details.codecctx)) < 0){
    //fprintf(stderr, "Couldn't get codec params for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }*/
//fprintf(stderr, "FRAME FRAME: %p\n", ncv->details.frame);
  // frame is set up in prep_details(), so that format can be set there, as
  // is necessary when it is prepared from inputs other than files. oframe
  // is set up whenever we convert to RGBA.
  if(ncvisual_decode(ncv)){
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return nullptr;
}

// iterate over the decoded frames, calling streamer() with curry for each.
// frames carry a presentation time relative to the beginning, so we get an
// initial timestamp, and check each frame against the elapsed time to sync
// up playback.
int ncvisual_stream(notcurses* nc, ncvisual* ncv, float timescale,
                    streamcb streamer, const struct ncvisual_options* vopts,
                    void* curry) {
  int frame = 1;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  uint64_t nsbegin = timespec_to_ns(&begin);
  bool usets = false;
  // each frame has a pkt_duration in milliseconds. keep the aggregate, in case
  // we don't have PTS available.
  uint64_t sum_duration = 0;
  ncplane* newn = NULL;
  ncvisual_options activevopts;
  memcpy(&activevopts, vopts, sizeof(*vopts));
  int ncerr;
  do{
    // codecctx seems to be off by a factor of 2 regularly. instead, go with
    // the time_base from the avformatctx.
    double tbase = av_q2d(ncv->details.fmtctx->streams[ncv->details.stream_index]->time_base);
    int64_t ts = ncv->details.frame->best_effort_timestamp;
    if(frame == 1 && ts){
      usets = true;
    }
    if(activevopts.n){
      ncplane_erase(activevopts.n); // new frame could be partially transparent
    }
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
    uint64_t duration = ncv->details.frame->pkt_duration * tbase * NANOSECS_IN_SEC;
//fprintf(stderr, "use: %u dur: %ju ts: %ju cctx: %f fctx: %f\n", usets, duration, ts, av_q2d(ncv->details.codecctx->time_base), av_q2d(ncv->details.fmtctx->streams[ncv->stream_index]->time_base));
    double schedns = nsbegin;
    if(usets){
      if(tbase == 0){
        tbase = duration;
      }
      schedns += ts * (tbase * timescale) * NANOSECS_IN_SEC;
    }else{
      sum_duration += (duration * timescale);
      schedns += sum_duration;
    }
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
  }while((ncerr = ncvisual_decode(ncv)) == 0);
  if(activevopts.n != vopts->n){
    ncplane_destroy(activevopts.n);
  }
  if(ncerr == 1){ // 1 indicates reaching EOF
    ncerr = 0;
  }
  return ncerr;
}

int ncvisual_decode_loop(ncvisual* ncv){
  int r = ncvisual_decode(ncv);
  if(r == 1){
    if(av_seek_frame(ncv->details.fmtctx, ncv->details.stream_index, 0, AVSEEK_FLAG_FRAME) < 0){
      // FIXME log error
      return -1;
    }
    if(ncvisual_decode(ncv) < 0){
      return -1;
    }
  }
  return r;
}

int ncvisual_blit(ncvisual* ncv, int rows, int cols, ncplane* n,
                  const struct blitset* bset, int placey, int placex,
                  int begy, int begx, int leny, int lenx,
                  bool blendcolors) {
  const AVFrame* inframe = ncv->details.oframe ? ncv->details.oframe : ncv->details.frame;
//fprintf(stderr, "inframe: %p oframe: %p frame: %p\n", inframe, ncv->details.oframe, ncv->details.frame);
  void* data = nullptr;
  int stride = 0;
  AVFrame* sframe = nullptr;
  const int targformat = AV_PIX_FMT_RGBA;
//fprintf(stderr, "got format: %d want format: %d\n", inframe->format, targformat);
  if(inframe && (cols != inframe->width || rows != inframe->height || inframe->format != targformat)){
//fprintf(stderr, "resize+render: %d/%d->%d/%d (%dX%d @ %dX%d, %d/%d)\n", inframe->height, inframe->width, rows, cols, begy, begx, placey, placex, leny, lenx);
    sframe = av_frame_alloc();
    if(sframe == nullptr){
//fprintf(stderr, "Couldn't allocate output frame for scaled frame\n");
      return -1;
    }
    //fprintf(stderr, "WHN NCV: %d/%d\n", inframe->width, inframe->height);
    ncv->details.swsctx = sws_getCachedContext(ncv->details.swsctx,
                                               ncv->cols, ncv->rows,
                                               static_cast<AVPixelFormat>(inframe->format),
                                               cols, rows,
                                               static_cast<AVPixelFormat>(targformat),
                                               SWS_LANCZOS, nullptr, nullptr, nullptr);
    if(ncv->details.swsctx == nullptr){
//fprintf(stderr, "Error retrieving details.swsctx\n");
      return -1;
    }
    memcpy(sframe, inframe, sizeof(*inframe));
    sframe->format = targformat;
    sframe->width = cols;
    sframe->height = rows;
    int size = av_image_alloc(sframe->data, sframe->linesize,
                              sframe->width, sframe->height,
                              static_cast<AVPixelFormat>(sframe->format),
                              IMGALLOCALIGN);
    if(size < 0){
//fprintf(stderr, "Error allocating visual data (%d X %d)\n", sframe->height, sframe->width);
      return -1;
    }
    int height = sws_scale(ncv->details.swsctx, (const uint8_t* const*)inframe->data,
                           inframe->linesize, 0, inframe->height, sframe->data,
                           sframe->linesize);
    if(height < 0){
//fprintf(stderr, "Error applying scaling (%d X %d)\n", inframe->height, inframe->width);
      return -1;
    }
    stride = sframe->linesize[0]; // FIXME check for others?
    data = sframe->data[0];
//fprintf(stderr, "scaled %d/%d to %d/%d (%d/%d)\n", ncv->rows, ncv->cols, rows, cols, sframe->height, sframe->width);
  }else{
    stride = ncv->rowstride;
    data = ncv->data;
  }
//fprintf(stderr, "place: %d/%d rows/cols: %d/%d %d/%d+%d/%d\n", placey, placex, rows, cols, begy, begx, leny, lenx);
  if(rgba_blit_dispatch(n, bset, placey, placex, stride, data, begy, begx,
                        leny, lenx, blendcolors) < 0){
//fprintf(stderr, "rgba dispatch failed!\n");
    if(sframe){
      av_freep(sframe->data);
      av_freep(&sframe);
    }
    return -1;
  }
  if(sframe){
    av_freep(sframe->data);
    av_freep(&sframe);
  }
  return 0;
}

auto ncvisual_details_seed(ncvisual* ncv) -> void {
  assert(nullptr == ncv->details.oframe);
  ncv->details.frame->data[0] = reinterpret_cast<uint8_t*>(ncv->data);
  ncv->details.frame->data[1] = nullptr;
  ncv->details.frame->linesize[0] = ncv->rowstride;
  ncv->details.frame->linesize[1] = 0;
  ncv->details.frame->width = ncv->cols;
  ncv->details.frame->height = ncv->rows;
  ncv->details.frame->format = AV_PIX_FMT_RGBA;
}

int ncvisual_init(int loglevel) {
  av_log_set_level(loglevel);
  // FIXME could also use av_log_set_callback() and capture the message...
  return 0;
}
#endif
