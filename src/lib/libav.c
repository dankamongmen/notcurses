#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include "notcurses.h"

int notcurses_visual_open(struct notcurses* nc __attribute__ ((unused)),
                          const char* filename){
  AVFormatContext* ps = NULL;
  int ret = avformat_open_input(&ps, filename, NULL, NULL);
  if(ret < 0){
    fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(ret));
    return ret;
  }
  if((ret = avformat_find_stream_info(ps, NULL)) < 0){
    fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(ret));
    avformat_close_input(&ps);
    return ret;
  }
av_dump_format(ps, 0, filename, false);
  AVPacket* packet = av_packet_alloc();
  if((ret = av_read_frame(ps, packet)) < 0){
    fprintf(stderr, "Error reading frame info from %s (%s)\n", filename,
            av_err2str(ret));
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  AVCodec* codec;
  if((ret = av_find_best_stream(ps, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0){
    fprintf(stderr, "Couldn't find visuals in %s (%s)\n", filename, av_err2str(ret));
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  if(codec == NULL){
    fprintf(stderr, "Couldn't find decoder for %s\n", filename);
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  /*
  stream = ret;
  AVStream* avstream = ps->streams[stream];
  AVCodecContext* codecctx = avstream->codec;
  AVCodec* codec = avcodec_find_decoder(codecctx->codec_id);
  if(codec == NULL){
    fprintf(stderr, "Couldn't find decoder for %s (%s)\n", filename);
    avcodec_close(codecctx);
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  */
  AVCodecContext* codecctx = avcodec_alloc_context3(codec);
  if((ret = avcodec_open2(codecctx, codec, NULL)) < 0){
    fprintf(stderr, "Couldn't open codec for %s (%s)\n", filename, av_err2str(ret));
    avcodec_free_context(&codecctx);
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  if((ret = avcodec_send_packet(codecctx, packet)) < 0){
    fprintf(stderr, "Error decoding packet from %s (%s)\n", filename,
            av_err2str(ret));
    avcodec_close(codecctx);
    avcodec_free_context(&codecctx);
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }
  AVFrame* frame = av_frame_alloc();
  if(frame == NULL){
    avcodec_close(codecctx);
    avcodec_free_context(&codecctx);
    av_packet_free(&packet);
    avformat_close_input(&ps);
    return -1;
  }

  // FIXME
  avcodec_close(codecctx);
  avcodec_free_context(&codecctx);
  av_frame_free(&frame);
  av_packet_free(&packet);
  avformat_close_input(&ps);
  return 0;
}
