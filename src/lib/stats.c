#include "internal.h"

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
  pthread_mutex_lock(&nc->statlock);
  memcpy(stats, &nc->stats, sizeof(*stats));
  pthread_mutex_unlock(&nc->statlock);
}

ncstats* notcurses_stats_alloc(const notcurses* nc __attribute__ ((unused))){
  return malloc(sizeof(ncstats));
}

void notcurses_stats_reset(notcurses* nc, ncstats* stats){
  pthread_mutex_lock(&nc->statlock);
  if(stats){
    memcpy(stats, &nc->stats, sizeof(*stats));
  }
  // add the stats to the stashed stats, so that we can show true totals on
  // shutdown in the closing banner
  ncstats* stash = &nc->stashed_stats;
  if(nc->stats.render_min_ns < stash->render_min_ns){
    stash->render_min_ns = nc->stats.render_min_ns;
  }
  if(nc->stats.render_min_bytes < stash->render_min_bytes){
    stash->render_min_bytes = nc->stats.render_min_bytes;
  }
  if(nc->stats.raster_min_ns < stash->raster_min_ns){
    stash->raster_min_ns = nc->stats.raster_min_ns;
  }
  if(nc->stats.writeout_min_ns < stash->writeout_min_ns){
    stash->writeout_min_ns = nc->stats.writeout_min_ns;
  }
  if(nc->stats.render_max_ns > stash->render_max_ns){
    stash->render_max_ns = nc->stats.render_max_ns;
  }
  if(nc->stats.render_max_bytes > stash->render_max_bytes){
    stash->render_max_bytes = nc->stats.render_max_bytes;
  }
  if(nc->stats.raster_max_ns > stash->raster_max_ns){
    stash->raster_max_ns = nc->stats.raster_max_ns;
  }
  if(nc->stats.writeout_max_ns > stash->writeout_max_ns){
    stash->writeout_max_ns = nc->stats.writeout_max_ns;
  }
  stash->writeout_ns += nc->stats.writeout_ns;
  stash->raster_ns += nc->stats.raster_ns;
  stash->render_ns += nc->stats.render_ns;
  stash->render_bytes += nc->stats.render_bytes;
  stash->failed_renders += nc->stats.failed_renders;
  stash->failed_writeouts += nc->stats.failed_writeouts;
  stash->renders += nc->stats.renders;
  stash->writeouts += nc->stats.writeouts;
  stash->cellelisions += nc->stats.cellelisions;
  stash->cellemissions += nc->stats.cellemissions;
  stash->fgelisions += nc->stats.fgelisions;
  stash->fgemissions += nc->stats.fgemissions;
  stash->bgelisions += nc->stats.bgelisions;
  stash->bgemissions += nc->stats.bgemissions;
  stash->defaultelisions += nc->stats.defaultelisions;
  stash->defaultemissions += nc->stats.defaultemissions;
  stash->refreshes += nc->stats.refreshes;
  stash->fbbytes = nc->stats.fbbytes;
  stash->planes = nc->stats.planes;
  reset_stats(&nc->stats);
  pthread_mutex_unlock(&nc->statlock);
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
    fprintf(stderr, "\n%ju render%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->renders, stats->renders == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    qprefix(stats->raster_ns, NANOSECS_IN_SEC, totalbuf, 0);
    qprefix(stats->raster_min_ns, NANOSECS_IN_SEC, minbuf, 0);
    qprefix(stats->raster_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
    qprefix(stats->writeouts ? stats->raster_ns / stats->writeouts : 0, NANOSECS_IN_SEC, avgbuf, 0);
    fprintf(stderr, "%ju raster%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->writeouts, stats->writeouts == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    qprefix(stats->writeout_ns, NANOSECS_IN_SEC, totalbuf, 0);
    qprefix(stats->writeout_min_ns, NANOSECS_IN_SEC, minbuf, 0);
    qprefix(stats->writeout_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
    qprefix(stats->writeouts ? stats->writeout_ns / stats->writeouts : 0,
            NANOSECS_IN_SEC, avgbuf, 0);
    fprintf(stderr, "%ju write%s, %ss (%ss min, %ss avg, %ss max)\n",
            stats->writeouts, stats->writeouts == 1 ? "" : "s",
            totalbuf, minbuf, avgbuf, maxbuf);
    bprefix(stats->render_bytes, 1, totalbuf, 1),
    bprefix(stats->render_min_bytes, 1, minbuf, 1),
    bprefix(stats->renders ? stats->render_bytes / stats->renders : 0, 1, avgbuf, 1);
    bprefix(stats->render_max_bytes, 1, maxbuf, 1),
    fprintf(stderr, "%sB (%sB min, %sB avg, %sB max)\n",
            totalbuf, minbuf, avgbuf, maxbuf);
  }
  if(stats->renders || stats->failed_renders){
    fprintf(stderr, "%ju failed render%s, %ju failed write%s, %ju refresh%s\n",
            stats->failed_renders,
            stats->failed_renders == 1 ? "" : "s",
            stats->failed_writeouts,
            stats->failed_writeouts == 1 ? "" : "s",
            stats->refreshes,
            stats->refreshes == 1 ? "" : "es");
    fprintf(stderr, "RGB emits:elides: def %ju:%ju fg %ju:%ju bg %ju:%ju\n",
            stats->defaultemissions,
            stats->defaultelisions,
            stats->fgemissions,
            stats->fgelisions,
            stats->bgemissions,
            stats->bgelisions);
    fprintf(stderr, "Cell emits:elides: %ju/%ju (%.2f%%) %.2f%% %.2f%% %.2f%%\n",
            stats->cellemissions, stats->cellelisions,
            (stats->cellemissions + stats->cellelisions) == 0 ? 0 :
            (stats->cellelisions * 100.0) / (stats->cellemissions + stats->cellelisions),
            (stats->defaultemissions + stats->defaultelisions) == 0 ? 0 :
            (stats->defaultelisions * 100.0) / (stats->defaultemissions + stats->defaultelisions),
            (stats->fgemissions + stats->fgelisions) == 0 ? 0 :
            (stats->fgelisions * 100.0) / (stats->fgemissions + stats->fgelisions),
            (stats->bgemissions + stats->bgelisions) == 0 ? 0 :
            (stats->bgelisions * 100.0) / (stats->bgemissions + stats->bgelisions));
  }
}
