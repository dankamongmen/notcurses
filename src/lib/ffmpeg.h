#ifndef NOTCURSES_FFMPEG
#define NOTCURSES_FFMPEG

#include "version.h"
#ifdef USE_FFMPEG

extern "C" {

#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include <libswscale/version.h>
#include <libavformat/version.h>
#include <libavformat/avformat.h>

} // extern "C"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;

typedef struct ncvisual_details {
  int packet_outstanding;
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;     // video codec context
  struct AVCodecContext* subtcodecctx; // subtitle codec context
  struct AVFrame* frame;               // frame as read
  struct AVFrame* oframe;              // RGBA frame
  struct AVCodec* codec;
  struct AVCodecParameters* cparams;
  struct AVCodec* subtcodec;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  AVSubtitle subtitle;
  int stream_index;        // match against this following av_read_frame()
  int sub_stream_index;    // subtitle stream index, can be < 0 if no subtitles
} ncvisual_details;

static inline auto
ncvisual_details_init(ncvisual_details* deets) -> int {
  memset(deets, 0, sizeof(*deets));
  deets->stream_index = -1;
  deets->sub_stream_index = -1;
  if((deets->frame = av_frame_alloc()) == nullptr){
    return -1;
  }
  return 0;
}

static inline auto
ncvisual_details_destroy(ncvisual_details* deets) -> void {
  avcodec_close(deets->codecctx);
  avcodec_free_context(&deets->codecctx);
  av_frame_free(&deets->frame);
  av_freep(&deets->oframe);
  //avcodec_parameters_free(&ncv->cparams);
  sws_freeContext(deets->swsctx);
  av_packet_free(&deets->packet);
  avformat_close_input(&deets->fmtctx);
  avsubtitle_free(&deets->subtitle);
}

#endif

#endif
