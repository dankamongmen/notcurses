#include <inttypes.h>
#include "internal.h"

// update timings for writeout. only call on success. call only under statlock.
void update_write_stats(const struct timespec* time1, const struct timespec* time0,
                        ncstats* stats, int bytes){
  if(bytes >= 0){
    const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
    if(elapsed > 0){ // don't count clearly incorrect information, egads
      ++stats->writeouts;
      stats->writeout_ns += elapsed;
      if(elapsed > stats->writeout_max_ns){
        stats->writeout_max_ns = elapsed;
      }
      if(elapsed < stats->writeout_min_ns){
        stats->writeout_min_ns = elapsed;
      }
    }
  }else{
    ++stats->failed_writeouts;
  }
}

// negative 'bytes' are ignored as failures. call only while holding statlock.
// we don't increment failed_renders here because 'bytes' < 0 actually indicates
// a rasterization failure -- we can't fail in rendering anymore.
void update_render_bytes(ncstats* stats, int bytes){
  if(bytes >= 0){
    stats->render_bytes += bytes;
    if(bytes > stats->render_max_bytes){
      stats->render_max_bytes = bytes;
    }
    if(bytes < stats->render_min_bytes){
      stats->render_min_bytes = bytes;
    }
  }
}

// call only while holding statlock.
void update_render_stats(const struct timespec* time1, const struct timespec* time0,
                         ncstats* stats){
  const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
  //fprintf(stderr, "Rendering took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    ++stats->renders;
    stats->render_ns += elapsed;
    if(elapsed > stats->render_max_ns){
      stats->render_max_ns = elapsed;
    }
    if(elapsed < stats->render_min_ns){
      stats->render_min_ns = elapsed;
    }
  }
}

// call only while holding statlock.
void update_raster_stats(const struct timespec* time1, const struct timespec* time0,
                         ncstats* stats){
  const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
  //fprintf(stderr, "Rasterizing took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    stats->raster_ns += elapsed;
    if(elapsed > stats->raster_max_ns){
      stats->raster_max_ns = elapsed;
    }
    if(elapsed < stats->raster_min_ns){
      stats->raster_min_ns = elapsed;
    }
  }
}

void reset_stats(ncstats* stats){
  uint64_t fbbytes = stats->fbbytes;
  unsigned planes = stats->planes;
  memset(stats, 0, sizeof(*stats));
  stats->render_min_ns = 1ull << 62u;
  stats->render_min_bytes = 1ull << 62u;
  stats->raster_min_ns = 1ull << 62u;
  stats->writeout_min_ns = 1ull << 62u;
  stats->fbbytes = fbbytes;
  stats->planes = planes;
}

void notcurses_stats(notcurses* nc, ncstats* stats){
  pthread_mutex_lock(&nc->stats.lock);
    memcpy(stats, &nc->stats, sizeof(*stats));
  pthread_mutex_unlock(&nc->stats.lock);
}

ncstats* notcurses_stats_alloc(const notcurses* nc __attribute__ ((unused))){
  ncstats* ret = malloc(sizeof(ncstats));
  if(ret == NULL){
    return NULL;
  }
  return ret;
}

void notcurses_stats_reset(notcurses* nc, ncstats* stats){
  pthread_mutex_lock(&nc->stats.lock);
    if(stats){
      memcpy(stats, &nc->stats, sizeof(*stats));
    }
    // add the stats to the stashed stats, so that we can show true totals on
    // shutdown in the closing banner
    ncstats* stash = &nc->stashed_stats;
    if(nc->stats.s.render_min_ns < stash->render_min_ns){
      stash->render_min_ns = nc->stats.s.render_min_ns;
    }
    if(nc->stats.s.render_min_bytes < stash->render_min_bytes){
      stash->render_min_bytes = nc->stats.s.render_min_bytes;
    }
    if(nc->stats.s.raster_min_ns < stash->raster_min_ns){
      stash->raster_min_ns = nc->stats.s.raster_min_ns;
    }
    if(nc->stats.s.writeout_min_ns < stash->writeout_min_ns){
      stash->writeout_min_ns = nc->stats.s.writeout_min_ns;
    }
    if(nc->stats.s.render_max_ns > stash->render_max_ns){
      stash->render_max_ns = nc->stats.s.render_max_ns;
    }
    if(nc->stats.s.render_max_bytes > stash->render_max_bytes){
      stash->render_max_bytes = nc->stats.s.render_max_bytes;
    }
    if(nc->stats.s.raster_max_ns > stash->raster_max_ns){
      stash->raster_max_ns = nc->stats.s.raster_max_ns;
    }
    if(nc->stats.s.writeout_max_ns > stash->writeout_max_ns){
      stash->writeout_max_ns = nc->stats.s.writeout_max_ns;
    }
    stash->writeout_ns += nc->stats.s.writeout_ns;
    stash->raster_ns += nc->stats.s.raster_ns;
    stash->render_ns += nc->stats.s.render_ns;
    stash->render_bytes += nc->stats.s.render_bytes;
    stash->failed_renders += nc->stats.s.failed_renders;
    stash->failed_writeouts += nc->stats.s.failed_writeouts;
    stash->renders += nc->stats.s.renders;
    stash->writeouts += nc->stats.s.writeouts;
    stash->cellelisions += nc->stats.s.cellelisions;
    stash->cellemissions += nc->stats.s.cellemissions;
    stash->fgelisions += nc->stats.s.fgelisions;
    stash->fgemissions += nc->stats.s.fgemissions;
    stash->bgelisions += nc->stats.s.bgelisions;
    stash->bgemissions += nc->stats.s.bgemissions;
    stash->defaultelisions += nc->stats.s.defaultelisions;
    stash->defaultemissions += nc->stats.s.defaultemissions;
    stash->refreshes += nc->stats.s.refreshes;
    stash->sprixelemissions += nc->stats.s.sprixelemissions;
    stash->sprixelelisions += nc->stats.s.sprixelelisions;
    stash->sprixelbytes += nc->stats.s.sprixelbytes;
    stash->appsync_updates += nc->stats.s.appsync_updates;
    stash->input_errors += nc->stats.s.input_errors;

    stash->fbbytes = nc->stats.s.fbbytes;
    stash->planes = nc->stats.s.planes;
    reset_stats(&nc->stats.s);
  pthread_mutex_unlock(&nc->stats.lock);
}

void summarize_stats(notcurses* nc){
  const ncstats *stats = &nc->stashed_stats;
  if(stats->renders){
    char totalbuf[BPREFIXSTRLEN + 1];
    char minbuf[BPREFIXSTRLEN + 1];
    char maxbuf[BPREFIXSTRLEN + 1];
    char avgbuf[BPREFIXSTRLEN + 1];
    qprefix(stats->render_ns, NANOSECS_IN_SEC, totalbuf, 0);
    qprefix(stats->render_min_ns, NANOSECS_IN_SEC, minbuf, 0);
    qprefix(stats->render_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
    qprefix(stats->render_ns / stats->renders, NANOSECS_IN_SEC, avgbuf, 0);
    ncfputc('\n', stdout);
    fprintf(stderr, "%"PRIu64" render%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->renders, stats->renders == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    qprefix(stats->raster_ns, NANOSECS_IN_SEC, totalbuf, 0);
    qprefix(stats->raster_min_ns, NANOSECS_IN_SEC, minbuf, 0);
    qprefix(stats->raster_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
    qprefix((stats->writeouts || stats->failed_writeouts) ?
            stats->raster_ns / (stats->writeouts + stats->failed_writeouts)
            : 0, NANOSECS_IN_SEC, avgbuf, 0);
    fprintf(stderr, "%"PRIu64" raster%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->writeouts, stats->writeouts == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    qprefix(stats->writeout_ns, NANOSECS_IN_SEC, totalbuf, 0);
    qprefix(stats->writeout_ns ? stats->writeout_min_ns : 0,
            NANOSECS_IN_SEC, minbuf, 0);
    qprefix(stats->writeout_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
    qprefix(stats->writeouts ? stats->writeout_ns / stats->writeouts : 0,
            NANOSECS_IN_SEC, avgbuf, 0);
    fprintf(stderr, "%"PRIu64" write%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->writeouts, stats->writeouts == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    bprefix(stats->render_bytes, 1, totalbuf, 1),
    bprefix(stats->render_bytes ? stats->render_min_bytes : 0,
            1, minbuf, 1),
    bprefix(stats->renders ? stats->render_bytes / stats->renders : 0, 1, avgbuf, 1);
    bprefix(stats->render_max_bytes, 1, maxbuf, 1),
    fprintf(stderr, "%sB (%sB min, %sB avg, %sB max)\n",
            totalbuf, minbuf, avgbuf, maxbuf);
  }
  if(stats->renders || stats->failed_renders){
    fprintf(stderr, "%"PRIu64" failed render%s, %"PRIu64" failed raster%s, %"PRIu64" refresh%s, %"PRIu64" input error%s\n",
            stats->failed_renders, stats->failed_renders == 1 ? "" : "s",
            stats->failed_writeouts, stats->failed_writeouts == 1 ? "" : "s",
            stats->refreshes, stats->refreshes == 1 ? "" : "es",
            stats->input_errors, stats->input_errors == 1 ? "" : "s");
    fprintf(stderr, "RGB emits:elides: def %"PRIu64":%"PRIu64" fg %"PRIu64":%"PRIu64" bg %"PRIu64":%"PRIu64"\n",
            stats->defaultemissions,
            stats->defaultelisions,
            stats->fgemissions,
            stats->fgelisions,
            stats->bgemissions,
            stats->bgelisions);
    fprintf(stderr, "Cell emits:elides: %"PRIu64":%"PRIu64" (%.2f%%) %.2f%% %.2f%% %.2f%%\n",
            stats->cellemissions, stats->cellelisions,
            (stats->cellemissions + stats->cellelisions) == 0 ? 0 :
            (stats->cellelisions * 100.0) / (stats->cellemissions + stats->cellelisions),
            (stats->defaultemissions + stats->defaultelisions) == 0 ? 0 :
            (stats->defaultelisions * 100.0) / (stats->defaultemissions + stats->defaultelisions),
            (stats->fgemissions + stats->fgelisions) == 0 ? 0 :
            (stats->fgelisions * 100.0) / (stats->fgemissions + stats->fgelisions),
            (stats->bgemissions + stats->bgelisions) == 0 ? 0 :
            (stats->bgelisions * 100.0) / (stats->bgemissions + stats->bgelisions));
    char totalbuf[BPREFIXSTRLEN + 1];
    bprefix(stats->sprixelbytes, 1, totalbuf, 1);
    fprintf(stderr, "Bitmap emits:elides: %"PRIu64":%"PRIu64" (%.2f%%) %sB (%.2f%%) SuM: %"PRIu64" (%.2f%%)\n",
            stats->sprixelemissions, stats->sprixelelisions,
            (stats->sprixelemissions + stats->sprixelelisions) == 0 ? 0 :
            (stats->sprixelelisions * 100.0) / (stats->sprixelemissions + stats->sprixelelisions),
            totalbuf,
            stats->render_bytes ? (stats->sprixelbytes * 100.0) / stats->render_bytes : 0,
            stats->appsync_updates,
            stats->writeouts ? stats->appsync_updates * 100.0 / stats->writeouts : 0);
  }
}
