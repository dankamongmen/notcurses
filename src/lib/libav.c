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
  avformat_free_context(ps);
  return 0;
}
