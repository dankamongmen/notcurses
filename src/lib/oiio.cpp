#include "internal.h"

#ifdef USE_OIIO
#include <OpenImageIO/imageio.h>
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

bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return true;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, int* averr){
  auto in = OIIO::ImageInput::open(filename);
  if(!in){
    return nullptr;
  }
  (void)nc;
  (void)averr;
  return NULL;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              int* averr, int y, int x, ncscale_e style){
  (void)nc;
  (void)filename;
  (void)averr;
  (void)y;
  (void)x;
  (void)style;
  return NULL;
}

struct AVFrame* ncvisual_decode(ncvisual* nc, int* averr){
  (void)nc;
  (void)averr;
  return NULL;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
  (void)ncv;
  (void)begy;
  (void)begx;
  (void)leny;
  (void)lenx;
  return -1;
}

int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, int* averr,
                    float timespec, streamcb streamer, void* curry){
  (void)nc;
  (void)ncv;
  (void)averr;
  (void)timespec;
  (void)streamer;
  (void)curry;
  return -1;
}

char* ncvisual_subtitle(const ncvisual* ncv){
  (void)ncv;
  return NULL;
}

int ncvisual_init(int loglevel){
  (void)loglevel;
  return 0; // allow success here
}
#endif
