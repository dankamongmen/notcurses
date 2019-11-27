#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include "notcurses.h"

typedef struct ncvisual {
  AVFormatContext* fmtctx;
  AVCodecContext* codecctx;
  AVFrame* frame;
  AVCodec* codec;
  AVPacket* packet;
  int packet_outstanding;
} ncvisual;

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
    av_packet_free(&ncv->packet);
    avformat_close_input(&ncv->fmtctx);
    free(ncv);
  }
}

AVFrame* ncvisual_decode(struct ncvisual* nc){
  int ret;
  if(nc->packet_outstanding){
    ret = avcodec_send_packet(nc->codecctx, nc->packet);
    if(ret < 0){
      fprintf(stderr, "Error processing AVPacket (%s)\n", av_err2str(ret));
      return NULL;
    }
    --nc->packet_outstanding;
  }
  ret = avcodec_receive_frame(nc->codecctx, nc->frame);
  if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
    return NULL; // FIXME do something smarter
  }else if(ret < 0){
    fprintf(stderr, "Error decoding AVPacket (%s)\n", av_err2str(ret));
    return NULL;
  }
  fprintf(stderr, "Got frame %05d\n", nc->codecctx->frame_number);
  return nc->frame;
}

ncvisual* notcurses_visual_open(struct notcurses* nc __attribute__ ((unused)),
                                const char* filename){
  ncvisual* ncv = ncvisual_create();
  if(ncv == NULL){
    fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    return NULL;
  }
  int ret = avformat_open_input(&ncv->fmtctx, filename, NULL, NULL);
  if(ret < 0){
    fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(ret));
    goto err;
  }
  if((ret = avformat_find_stream_info(ncv->fmtctx, NULL)) < 0){
    fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(ret));
    goto err;
  }
av_dump_format(ncv->fmtctx, 0, filename, false);
  if((ncv->packet = av_packet_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate packet for %s\n", filename);
    goto err;
  }
  if((ret = av_read_frame(ncv->fmtctx, ncv->packet)) < 0){
    fprintf(stderr, "Error reading frame info from %s (%s)\n", filename,
            av_err2str(ret));
    goto err;
  }
  if((ret = av_find_best_stream(ncv->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ncv->codec, 0)) < 0){
    fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(ret));
    goto err;
  }
  if(ncv->codec == NULL){
    fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    goto err;
  }
  if((ncv->codecctx = avcodec_alloc_context3(ncv->codec)) == NULL){
    fprintf(stderr, "Couldn't allocate decoder for %s\n", filename);
    goto err;
  }
  if((ret = avcodec_open2(ncv->codecctx, ncv->codec, NULL)) < 0){
    fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(ret));
    goto err;
  }
  if((ret = avcodec_send_packet(ncv->codecctx, ncv->packet)) < 0){
    fprintf(stderr, "Error decoding packet from %s (%s)\n", filename,
            av_err2str(ret));
    goto err;
  }
  ++ncv->packet_outstanding;
  if((ncv->frame = av_frame_alloc()) == NULL){
    fprintf(stderr, "Couldn't allocate frame for %s\n", filename);
    goto err;
  }
  return ncv;

err:
  ncvisual_destroy(ncv);
  return NULL;
}
