#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include "notcurses.h"
#include "internal.h"

ncplane* ncvisual_plane(ncvisual* ncv){
  return ncv->ncp;
}

static ncvisual*
ncvisual_create(void){
  ncvisual* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return NULL;
  }
  memset(ret, 0, sizeof(*ret));
  return ret;
}

void ncvisual_destroy(ncvisual* ncv){
  if(ncv){
    avcodec_close(ncv->codecctx);
    avcodec_free_context(&ncv->codecctx);
    av_frame_free(&ncv->frame);
    // av_frame_free(&ncv->oframe); FIXME
    avcodec_parameters_free(&ncv->cparams);
    sws_freeContext(ncv->swsctx);
    av_packet_free(&ncv->packet);
    av_packet_free(&ncv->subtitle);
    avformat_close_input(&ncv->fmtctx);
    if(ncv->ncobj && ncv->ncp){
      ncplane_destroy(ncv->ncp);
    }
    free(ncv);
  }
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

AVFrame* ncvisual_decode(struct ncvisual* nc, int* averr){
  bool have_frame = false;
  bool unref = false;
  do{
    do{
      if(nc->packet_outstanding){
        break;
      }
      if(unref){
  // fprintf(stderr, "stream index %d != %d\n", nc->packet->stream_index, nc->stream_index);
        av_packet_unref(nc->packet);
      }
      if((*averr = av_read_frame(nc->fmtctx, nc->packet)) < 0){
        /*if(averr != AVERROR_EOF){
          fprintf(stderr, "Error reading frame info (%s)\n", av_err2str(*averr));
        }*/
        return NULL;
      }
      unref = true;
    }while(nc->packet->stream_index != nc->stream_index);
    ++nc->packet_outstanding;
    *averr = avcodec_send_packet(nc->codecctx, nc->packet);
    if(*averr < 0){
      //fprintf(stderr, "Error processing AVPacket (%s)\n", av_err2str(*averr));
      return ncvisual_decode(nc, averr);
    }
    --nc->packet_outstanding;
    *averr = avcodec_receive_frame(nc->codecctx, nc->frame);
    if(*averr >= 0){
      have_frame = true;
    }else if(*averr == AVERROR(EAGAIN) || *averr == AVERROR_EOF){
      have_frame = false;
    }else if(*averr < 0){
      //fprintf(stderr, "Error decoding AVPacket (%s)\n", av_err2str(*averr));
      return NULL;
    }
  }while(!have_frame);
//print_frame_summary(nc->codecctx, nc->frame);
#define IMGALLOCALIGN 32
  if(nc->ncp == NULL){ // create plane
    int rows, cols;
    if(nc->style == NCSCALE_NONE){
      rows = nc->frame->height / 2;
      cols = nc->frame->width;
    }else{ // FIXME differentiate between scale/stretch
      notcurses_term_dim_yx(nc->ncobj, &rows, &cols);
      if(nc->placey >= rows || nc->placex >= cols){
        return NULL;
      }
      rows -= nc->placey;
      cols -= nc->placex;
    }
    nc->dstwidth = cols;
    nc->dstheight = rows * 2;
    nc->ncp = notcurses_newplane(nc->ncobj, rows, cols, nc->placey, nc->placex, nc);
    if(nc->ncp == NULL){
      *averr = AVERROR(ENOMEM);
      return NULL;
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
    return NULL;
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
    return NULL;
  }
  int height = sws_scale(nc->swsctx, (const uint8_t* const*)nc->frame->data,
                         nc->frame->linesize, 0,
                         nc->frame->height, nc->oframe->data, nc->oframe->linesize);
  if(height < 0){
    //fprintf(stderr, "Error applying scaling (%s)\n", av_err2str(height));
    return NULL;
  }
//print_frame_summary(nc->codecctx, nc->oframe);
#undef IMGALLOCALIGN
  av_frame_unref(nc->frame);
  return nc->oframe;
}

static ncvisual*
ncvisual_open(const char* filename, int* averr){
  ncvisual* ncv = ncvisual_create();
  if(ncv == NULL){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *averr = AVERROR(ENOMEM);
    return NULL;
  }
  memset(ncv, 0, sizeof(*ncv));
  *averr = avformat_open_input(&ncv->fmtctx, filename, NULL, NULL);
  if(*averr < 0){
    // fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((*averr = avformat_find_stream_info(ncv->fmtctx, NULL)) < 0){
    /*fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(*averr));*/
    goto err;
  }
//av_dump_format(ncv->fmtctx, 0, filename, false);
  if((*averr = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, &ncv->subtcodec, 0)) >= 0){
    if((ncv->subtitle = av_packet_alloc()) == NULL){
      // fprintf(stderr, "Couldn't allocate subtitles for %s\n", filename);
      *averr = AVERROR(ENOMEM);
      goto err;
    }
  }
  if((ncv->packet = av_packet_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->codec, 0)) < 0){
    // fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  ncv->stream_index = *averr;
  if(ncv->codec == NULL){
    //fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  if((ncv->codecctx = avcodec_alloc_context3(ncv->codec)) == NULL){
    //fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = avcodec_open2(ncv->codecctx, ncv->codec, NULL)) < 0){
    //fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((ncv->cparams = avcodec_parameters_alloc()) == NULL){
    //fprintf(stderr, "Couldn't allocate codec params for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = avcodec_parameters_from_context(ncv->cparams, ncv->codecctx)) < 0){
    //fprintf(stderr, "Couldn't get codec params for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((ncv->frame = av_frame_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate frame for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((ncv->oframe = av_frame_alloc()) == NULL){
    // fprintf(stderr, "Couldn't allocate output frame for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return NULL;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, int* averr){
  ncvisual* ncv = ncvisual_open(filename, averr);
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
                              int* averr, int y, int x, ncscale_e style){
  ncvisual* ncv = ncvisual_open(filename, averr);
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
  if(begy < 0 || begx < 0 || lenx < 0 || leny < 0){
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
  if(begx + lenx > f->width || begy + leny > f->height){
    return -1;
  }
  if(lenx == 0){
    lenx = f->width - begx;
  }
  if(leny == 0){
    leny = f->height - begy;
  }
  int x, y;
  int dimy, dimx;
  ncplane_dim_yx(ncv->ncp, &dimy, &dimx);
  ncplane_cursor_move_yx(ncv->ncp, 0, 0);
  const int linesize = f->linesize[0];
  const unsigned char* data = f->data[0];
  // y and x are actual plane coordinates. each row corresponds to two rows of
  // the input (scaled) frame (columns are 1:1). we track the row of the
  // visual via visy.
  int visy = begy;
//fprintf(stderr, "render: %dx%d:%d+%d of %d/%d -> %dx%d\n", begy, begx, leny, lenx, f->height, f->width, dimy, dimx);
  for(y = ncv->placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 2){
    if(ncplane_cursor_move_yx(ncv->ncp, y, ncv->placex)){
      return -1;
    }
    int visx = begx;
    for(x = ncv->placex ; visx < (begx + lenx) && x < dimx ; ++x, ++visx){
      int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(f->format));
      const unsigned char* rgbbase_up = data + (linesize * visy) + (visx * bpp / CHAR_BIT);
      const unsigned char* rgbbase_down = data + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT);
/*fprintf(stderr, "[%04d/%04d] %p bpp: %d lsize: %d %02x %02x %02x %02x\n",
        y, x, rgbbase, bpp, linesize, rgbbase[0], rgbbase[1], rgbbase[2], rgbbase[3]);*/
      cell c = CELL_TRIVIAL_INITIALIZER;
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      if(!rgbbase_up[3] || !rgbbase_down[3]){
        cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
        if(!rgbbase_up[3] && !rgbbase_down[3]){
          if(cell_load(ncv->ncp, &c, " ") <= 0){
            return -1;
          }
          cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
        }else if(!rgbbase_up[3]){ // down has the color
          if(cell_load(ncv->ncp, &c, "\u2584") <= 0){ // lower half block
            return -1;
          }
          cell_set_fg_rgb(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
        }else{ // up has the color
          if(cell_load(ncv->ncp, &c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg_rgb(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        }
      }else{
        cell_set_fg_rgb(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        cell_set_bg_rgb(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
        if(cell_load(ncv->ncp, &c, "\u2580") <= 0){ // upper half block
          return -1;
        }
      }
      if(ncplane_putc(ncv->ncp, &c) <= 0){
        cell_release(ncv->ncp, &c);
        return -1;
      }
      cell_release(ncv->ncp, &c);
    }
  }
  return 0;
}

int ncvisual_stream(notcurses* nc, ncvisual* ncv, int* averr, streamcb streamer){
  ncplane* n = ncv->ncp;
  int frame = 1;
  AVFrame* avf;
  struct timespec start;
  // FIXME should keep a start time and cumulative time; this will push things
  // out on a loaded machine
  while(clock_gettime(CLOCK_MONOTONIC, &start),
        (avf = ncvisual_decode(ncv, averr)) ){
    ncplane_cursor_move_yx(n, 0, 0);
    if(ncvisual_render(ncv, 0, 0, 0, 0)){
      return -1;
    }
    if(streamer){
      int r = streamer(nc, ncv);
      if(r){
        return r;
      }
    }
    ++frame;
    uint64_t ns = avf->pkt_duration * 1000000;
    struct timespec interval = {
      .tv_sec = start.tv_sec + (long)(ns / 1000000000),
      .tv_nsec = start.tv_nsec + (long)(ns % 1000000000),
    };
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &interval, NULL);
  }
  if(*averr == AVERROR_EOF){
    return 0;
  }
  return -1;
}

int ncvisual_init(void){
  av_log_set_level(AV_LOG_QUIET); // FIXME make this configurable?
  // FIXME could also use av_log_set_callback() and capture the message...
  return 0;
}
