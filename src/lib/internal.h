#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
struct SwsContext;
struct ncplane;

typedef struct ncvisual {
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;
  struct AVFrame* frame;
  struct AVFrame* oframe;
  struct AVCodec* codec;
  struct AVCodecParameters* cparams;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  int packet_outstanding;
  int dstwidth, dstheight;
  struct ncplane* ncp;
} ncvisual;

#ifdef __cplusplus
}
#endif

#endif
