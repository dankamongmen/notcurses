#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include "notcurses.h"
#include "internal.h"

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
    av_frame_free(&ncv->oframe);
    avcodec_parameters_free(&ncv->cparams);
    sws_freeContext(ncv->swsctx);
    av_packet_free(&ncv->packet);
    avformat_close_input(&ncv->fmtctx);
    free(ncv);
  }
}

static void
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
}

AVFrame* ncvisual_decode(struct ncvisual* nc, int* averr){
fprintf(stderr, "\n*********************running decode+scale\n");
  if(nc->packet_outstanding){
    *averr = avcodec_send_packet(nc->codecctx, nc->packet);
    if(*averr < 0){
      fprintf(stderr, "Error processing AVPacket (%s)\n", av_err2str(*averr));
      return NULL;
    }
    --nc->packet_outstanding;
  }
  *averr = avcodec_receive_frame(nc->codecctx, nc->frame);
  if(*averr == AVERROR(EAGAIN) || *averr == AVERROR_EOF){
    return NULL; // FIXME do something smarter
  }else if(*averr < 0){
    fprintf(stderr, "Error decoding AVPacket (%s)\n", av_err2str(*averr));
    return NULL;
  }
print_frame_summary(nc->codecctx, nc->frame);
#define IMGALLOCALIGN 32
  nc->swsctx = sws_getCachedContext(nc->swsctx,
                                    nc->frame->width,
                                    nc->frame->height,
                                    nc->frame->format,
                                    nc->dstwidth,
                                    nc->dstheight,
                                    AV_PIX_FMT_RGB24,
                                    SWS_LANCZOS,
                                    NULL, NULL, NULL);
  if(nc->swsctx == NULL){
    fprintf(stderr, "Error retrieving swsctx (%s)\n", av_err2str(*averr));
    return NULL;
  }
  nc->oframe->format = AV_PIX_FMT_RGB24;
  nc->oframe->width = nc->dstwidth;
  nc->oframe->height = nc->dstheight;
  if((*averr = av_image_alloc(nc->oframe->data, nc->oframe->linesize,
                              nc->oframe->width, nc->oframe->height,
                              nc->oframe->format, IMGALLOCALIGN)) < 0){
    fprintf(stderr, "Error allocating visual data (%s)\n", av_err2str(*averr));
    av_frame_free(&nc->oframe);
    return NULL;
  }
fprintf(stderr, "ALLOCATED %d BYTES\n", *averr);
  *averr = sws_scale(nc->swsctx, (const uint8_t* const*)nc->frame->data,
                     nc->frame->linesize, 0,
                     nc->frame->height, nc->oframe->data, nc->oframe->linesize);
  if(*averr < 0){
    fprintf(stderr, "Error applying scaling (%s)\n", av_err2str(*averr));
    av_frame_free(&nc->oframe);
    return NULL;
  }
print_frame_summary(nc->codecctx, nc->oframe);
#undef IMGALLOCALIGN
  return nc->oframe;
}

ncvisual* ncplane_visual_open(struct ncplane* nc, const char* filename, int* averr){
  ncvisual* ncv = ncvisual_create();
  if(ncv == NULL){
    fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *averr = AVERROR(ENOMEM);
    return NULL;
  }
  memset(ncv, 0, sizeof(*ncv));
  ncv->ncp = nc;
  ncplane_dim_yx(nc, &ncv->dstheight, &ncv->dstwidth);
  *averr = avformat_open_input(&ncv->fmtctx, filename, NULL, NULL);
  if(*averr < 0){
    fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((*averr = avformat_find_stream_info(ncv->fmtctx, NULL)) < 0){
    fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(*averr));
    goto err;
  }
// av_dump_format(ncv->fmtctx, 0, filename, false);
  if((ncv->packet = av_packet_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = av_read_frame(ncv->fmtctx, ncv->packet)) < 0){
    fprintf(stderr, "Error reading frame info from %s (%s)\n", filename,
            av_err2str(*averr));
    goto err;
  }
  if((*averr = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->codec, 0)) < 0){
    fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if(ncv->codec == NULL){
    fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  if((ncv->codecctx = avcodec_alloc_context3(ncv->codec)) == NULL){
    fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = avcodec_open2(ncv->codecctx, ncv->codec, NULL)) < 0){
    fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((ncv->cparams = avcodec_parameters_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate codec params for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((*averr = avcodec_parameters_from_context(ncv->cparams, ncv->codecctx)) < 0){
    fprintf(stderr, "Couldn't get codec params for %s (%s)\n", filename, av_err2str(*averr));
    goto err;
  }
  if((*averr = avcodec_send_packet(ncv->codecctx, ncv->packet)) < 0){
    fprintf(stderr, "Error decoding packet from %s (%s)\n", filename,
            av_err2str(*averr));
    goto err;
  }
  ++ncv->packet_outstanding;
  if((ncv->frame = av_frame_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate frame for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  if((ncv->oframe = av_frame_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate output frame for %s\n", filename);
    *averr = AVERROR(ENOMEM);
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return NULL;
}
