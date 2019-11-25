#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include "notcurses.h"

int notcurses_image_open(struct notcurses* nc, const char* filename){
  AVFormatContext* ps = NULL;
  int ret = avformat_open_input(&ps, filename, NULL, NULL);
  if(ret < 0){
    fprintf(stderr, "Couldn't open %s (%s)\n", filename, av_err2str(ret));
    return ret;
  }
  if((ret = avformat_find_stream_info(ps, NULL)) < 0){
    fprintf(stderr, "Error extracting stream info from %s (%s)\n", filename,
            av_err2str(ret));
    avformat_free_context(ps);
    return ret;
  }
  // av_dump_format(ps, 0, filename, false);
  AVPacket* packet = av_packet_alloc();
  if((ret = av_read_frame(ps, packet)) < 0){
    fprintf(stderr, "Error reading frame info from %s (%s)\n", filename,
            av_err2str(ret));
    av_packet_free(&packet);
    avformat_free_context(ps);
    return -1;
  }
  // FIXME
  av_packet_free(&packet);
  avformat_free_context(ps);
  return 0;
}
