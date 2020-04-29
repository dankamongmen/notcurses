#include "version.h"
#ifdef USE_OIIO
#include <OpenImageIO/filter.h>
#include <OpenImageIO/version.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include "internal.h"

typedef struct ncvisual {
  int packet_outstanding;
  int dstwidth, dstheight;
  float timescale;         // scale frame duration by this value
  std::unique_ptr<OIIO::ImageInput> image;  // must be close()d
  std::unique_ptr<OIIO::ImageBuf> raw;
  char* filename;
  bool did_scaling;
  uint64_t framenum;
  OIIO::ImageBuf scaled;   // in use IFF did_scaling;
  std::unique_ptr<uint32_t[]> frame;
  ncplane* ncp;
  // if we're creating the plane based off the first frame's dimensions, these
  // describe where the plane ought be placed, and how it ought be sized. this
  // path sets ncobj. ncvisual_destroy() ought in that case kill the ncplane.
  int placex, placey;
  ncscale_e style;         // none, scale, or stretch
  struct notcurses* ncobj; // set iff this ncvisual "owns" its ncplane
} ncvisual;

ncplane* ncvisual_plane(ncvisual* ncv){
  return ncv->ncp;
}

bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return true;
}

static ncvisual*
ncvisual_create(const char* filename, float timescale){
  auto ret = new ncvisual;
  if(ret == nullptr){
    return nullptr;
  }
  ret->packet_outstanding = 0;
  ret->dstwidth = ret->dstheight = 0;
  ret->framenum = 0;
  ret->timescale = timescale;
  ret->image = nullptr;
  ret->ncp = nullptr;
  ret->placex = ret->placey = 0;
  ret->style = NCSCALE_NONE;
  ret->ncobj = nullptr;
  ret->frame = nullptr;
  ret->raw = nullptr;
  ret->filename = strdup(filename);
  return ret;
}

static ncvisual*
ncvisual_open(const char* filename, nc_err_e* err){
  ncvisual* ncv = ncvisual_create(filename, 1);
  if(ncv == nullptr){
    *err = NCERR_NOMEM;
    return nullptr;
  }
  ncv->image = OIIO::ImageInput::open(filename);
  if(!ncv->image){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *err = NCERR_DECODE;
    return nullptr;
  }
/*const auto &spec = ncv->image->spec_dimensions();
std::cout << "Opened " << filename << ": " << spec.height << "x" <<
spec.width << "@" << spec.nchannels << " (" << spec.format << ")" << std::endl;*/
  return ncv;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, nc_err_e* ncerr){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == nullptr){
    *ncerr = NCERR_NOMEM;
    return nullptr;
  }
  ncplane_dim_yx(nc, &ncv->dstheight, &ncv->dstwidth);
  ncv->dstheight *= 2;
  ncv->ncp = nc;
  ncv->style = NCSCALE_STRETCH;
  ncv->ncobj = nullptr;
  return ncv;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              nc_err_e* ncerr, int y, int x, ncscale_e style){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == nullptr){
    return nullptr;
  }
  ncv->placey = y;
  ncv->placex = x;
  ncv->style = style;
  ncv->ncobj = nc;
  ncv->ncp = nullptr;
  ncv->ncobj = nc;
  return ncv;
}

nc_err_e ncvisual_decode(ncvisual* nc){
//fprintf(stderr, "current subimage: %d frame: %p\n", nc->image->current_subimage(), nc->frame.get());
  const auto &spec = nc->image->spec_dimensions(nc->framenum);
  if(nc->frame){
//fprintf(stderr, "seeking subimage: %d\n", nc->image->current_subimage() + 1);
    OIIO::ImageSpec newspec;
    if(!nc->image->seek_subimage(nc->image->current_subimage() + 1, 0, newspec)){
       return NCERR_EOF;
    }
    // FIXME check newspec vis-a-vis image->spec()?
  }
//fprintf(stderr, "SUBIMAGE: %d\n", nc->image->current_subimage());
  nc->did_scaling = false;
  auto pixels = spec.width * spec.height;// * spec.nchannels;
  if(spec.nchannels < 3 || spec.nchannels > 4){
    return NCERR_DECODE; // FIXME get some to test with
  }
  nc->frame = std::make_unique<uint32_t[]>(pixels);
  if(spec.nchannels == 3){ // FIXME replace with channel shuffle
    std::fill(nc->frame.get(), nc->frame.get() + pixels, 0xfffffffful);
  }
//fprintf(stderr, "READING: %d %ju\n", nc->image->current_subimage(), nc->framenum);
  if(!nc->image->read_image(nc->framenum++, 0, 0, spec.nchannels, OIIO::TypeDesc(OIIO::TypeDesc::UINT8, 4), nc->frame.get(), 4)){
    return NCERR_DECODE;
  }
//fprintf(stderr, "READ: %d %ju\n", nc->image->current_subimage(), nc->framenum);
/*for(int i = 0 ; i < pixels ; ++i){
  //fprintf(stderr, "%06d %02x %02x %02x %02x\n", i,
  fprintf(stderr, "%06d %d %d %d %d\n", i,
      (nc->frame[i]) & 0xff,
      (nc->frame[i] >> 8) & 0xff,
      (nc->frame[i] >> 16) & 0xff,
      nc->frame[i] >> 24
      );
}*/
  OIIO::ImageSpec rgbaspec = spec;
  rgbaspec.nchannels = 4;
  nc->raw = std::make_unique<OIIO::ImageBuf>(rgbaspec, nc->frame.get());
//fprintf(stderr, "SUBS: %d\n", nc->raw->nsubimages());
  int rows, cols;
  if(nc->ncp == nullptr){ // create plane
    if(nc->style == NCSCALE_NONE){
      rows = spec.height / 2;
      cols = spec.width;
    }else{ // FIXME differentiate between scale/stretch
      notcurses_term_dim_yx(nc->ncobj, &rows, &cols);
      if(nc->placey >= rows || nc->placex >= cols){
        return NCERR_DECODE;
      }
      rows -= nc->placey;
      cols -= nc->placex;
    }
    nc->dstwidth = cols;
    nc->dstheight = rows * 2;
    nc->ncp = ncplane_new(nc->ncobj, rows, cols, nc->placey, nc->placex, nullptr);
    nc->placey = 0;
    nc->placex = 0;
    if(nc->ncp == nullptr){
      return NCERR_NOMEM;
    }
  }else{ // check for resize
    ncplane_dim_yx(nc->ncp, &rows, &cols);
    if(rows != nc->dstheight / 2 || cols != nc->dstwidth){
      nc->dstheight = rows * 2;
      nc->dstwidth = cols;
    }
  }
  if(nc->dstwidth != spec.width || nc->dstheight != spec.height){ // scale it
    OIIO::ROI roi(0, nc->dstwidth, 0, nc->dstheight, 0, 1, 0, 4);
    if(!OIIO::ImageBufAlgo::resize(nc->scaled, *nc->raw, "", 0, roi)){
      // FIXME
    }
    nc->did_scaling = true;
  }
  return NCERR_SUCCESS;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
//fprintf(stderr, "render %dx%d+%dx%d\n", begy, begx, leny, lenx);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return -1;
  }
  if(ncv->frame == nullptr){
    return -1;
  }
  const auto &spec = ncv->did_scaling ? ncv->scaled.spec() : ncv->raw->spec();
  const void* pixels = ncv->did_scaling ? ncv->scaled.localpixels() : ncv->raw->localpixels();
//fprintf(stderr, "render %d/%d to %dx%d+%dx%d\n", f->height, f->width, begy, begx, leny, lenx);
  if(begx >= spec.width || begy >= spec.height){
    return -1;
  }
  if(lenx == -1){ // -1 means "to the end"; use all space available
    lenx = spec.width - begx;
  }
  if(leny == -1){
    leny = spec.height - begy;
  }
  if(lenx == 0 || leny == 0){ // no need to draw zero-size object, exit
    return 0;
  }
  if(begx + lenx > spec.width || begy + leny > spec.height){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(ncv->ncp, &dimy, &dimx);
  ncplane_cursor_move_yx(ncv->ncp, 0, 0);
  // y and x are actual plane coordinates. each row corresponds to two rows of
  // the input (scaled) frame (columns are 1:1). we track the row of the
  // visual via visy.
//fprintf(stderr, "render: %dx%d:%d+%d of %d/%d -> %dx%d\n", begy, begx, leny, lenx, f->height, f->width, dimy, dimx);
  const int linesize = spec.width * 4;
  int ret = ncblit_rgba(ncv->ncp, ncv->placey, ncv->placex, linesize,
                        pixels, begy, begx, leny, lenx);
  //av_frame_unref(ncv->oframe);
  return ret;
}

int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, nc_err_e* ncerr,
                    float timescale, streamcb streamer, void* curry){
  int frame = 1;
  ncv->timescale = timescale;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  //uint64_t nsbegin = timespec_to_ns(&begin);
  //bool usets = false;
  // each frame has a pkt_duration in milliseconds. keep the aggregate, in case
  // we don't have PTS available.
  //uint64_t sum_duration = 0;
  while((*ncerr = ncvisual_decode(ncv)) == NCERR_SUCCESS){
    /* codecctx seems to be off by a factor of 2 regularly. instead, go with
    // the time_base from the avformatctx.
    double tbase = av_q2d(ncv->fmtctx->streams[ncv->stream_index]->time_base);
    int64_t ts = ncv->oframe->best_effort_timestamp;
    if(frame == 1 && ts){
      usets = true;
    }*/
    if(ncvisual_render(ncv, 0, 0, -1, -1) < 0){
      return -1;
    }
    if(streamer){
      int r = streamer(nc, ncv, curry);
      if(r){
        return r;
      }
    }
    ++frame;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    /*uint64_t nsnow = timespec_to_ns(&now);
    struct timespec interval;
    uint64_t duration = ncv->oframe->pkt_duration * tbase * NANOSECS_IN_SEC;
    sum_duration += (duration * ncv->timescale);
//fprintf(stderr, "use: %u dur: %ju ts: %ju cctx: %f fctx: %f\n", usets, duration, ts, av_q2d(ncv->codecctx->time_base), av_q2d(ncv->fmtctx->streams[ncv->stream_index]->time_base));
    double schedns = nsbegin;
    if(usets){
      if(tbase == 0){
        tbase = duration;
      }
      schedns += ts * (tbase * ncv->timescale) * NANOSECS_IN_SEC;
    }else{
      schedns += sum_duration;
    }
    if(nsnow < schedns){
      ns_to_timespec(schedns - nsnow, &interval);
      nanosleep(&interval, nullptr);
    }*/
  }
  if(*ncerr == NCERR_EOF){
    return 0;
  }
  return -1;
}

char* ncvisual_subtitle(const ncvisual* ncv){ // no support in OIIO
  (void)ncv;
  return nullptr;
}

int ncvisual_init(int loglevel){
  // FIXME set OIIO global attribute "debug" based on loglevel
  (void)loglevel;
  // FIXME check OIIO_VERSION_STRING components against linked openimageio_version()
  return 0; // allow success here
}

void ncvisual_destroy(ncvisual* ncv){
  if(ncv){
    ncv->image->close();
    if(ncv->ncobj){
      ncplane_destroy(ncv->ncp);
    }
    free(ncv->filename);
    delete ncv;
  }
}

// FIXME would be nice to have OIIO::attributes("libraries") in here
const char* oiio_version(void){
  return OIIO_VERSION_STRING;
}

#endif
