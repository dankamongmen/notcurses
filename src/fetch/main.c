#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/param.h>
#include <sys/types.h>
#if defined(__linux__) || defined(__gnu_hurd__)
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#elif !defined(__MINGW64__)
#include <sys/sysctl.h>
#include <sys/utsname.h>
#else
#include <sysinfoapi.h>
#endif
#include <notcurses/notcurses.h>
#include "compat/compat.h"
#include "ncart.h"

typedef struct distro_info {
  const char* name;            // must match 'lsb_release -i'
  const char* logofile;        // kept at original aspect ratio, lain atop bg
} distro_info;

typedef struct fetched_info {
  char* username;              // we borrow a reference
  char* hostname; 
  const distro_info* distro;
  char* logo;                  // strdup() from /etc/os-release
  char* distro_pretty;         // strdup() from /etc/os-release
  char* kernel;                // strdup(uname(2)->name)
  char* kernver;               // strdup(uname(2)->version);
  char* desktop;               // getenv("XDG_CURRENT_DESKTOP")
  const char* shell;           // getenv("SHELL")
  char* term;                  // notcurses_detected_terminal(), heap-alloced
  char* lang;                  // getenv("LANG")
  int dimy, dimx;              // extracted from xrandr
  char* cpu_model;             // FIXME don't handle hetero setups yet
  int core_count;
  // if there is no other logo found, fall back to a logo filched from neofetch
  const char* neologo;         // text with color substitution templates
} fetched_info;

static void
free_fetched_info(fetched_info* fi){
  free(fi->cpu_model);
  free(fi->hostname);
  free(fi->username);
  free(fi->kernel);
  free(fi->kernver);
  free(fi->distro_pretty);
  free(fi->term);
}

static int
fetch_env_vars(struct notcurses* nc, fetched_info* fi){
  fi->desktop = getenv("XDG_CURRENT_DESKTOP");
  fi->shell = getenv("SHELL");
  fi->term = notcurses_detected_terminal(nc);
  fi->lang = getenv("LANG");
  return 0;
}

static distro_info distros[] = {
  {
    .name = "arch",
    // from core/filesystem
    .logofile = "/usr/share/pixmaps/archlinux.png",
  }, {
    .name = "artix",
    // from system/filesystem
    .logofile = "/usr/share/pixmaps/artixlinux-logo.png",
  }, {
    .name = "debian",
    // from desktop-base package
    .logofile = "/usr/share/desktop-base/debian-logos/logo-text-256.png",
  }, {
    .name = "fedora",
    // from redhat-lsb-core package
    .logofile = "/usr/share/pixmaps/fedora-logo.png",
  }, {
    .name = NULL,
    .logofile = NULL,
  },
};

static int
fetch_bsd_cpuinfo(fetched_info* fi){
#if defined(__linux__) || defined(__gnu_hurd__) || defined(__MINGW64__)
  (void)fi;
#else
  size_t len = sizeof(fi->core_count);
  int mib[2] = { CTL_HW, HW_NCPU };
  if(sysctl(mib, sizeof(mib) / sizeof(*mib), &fi->core_count, &len, NULL, 0)){
    fprintf(stderr, "Coudln't acquire CTL_HW+HW_NCPU sysctl (%s)\n", strerror(errno));
    return -1;
  }
  mib[1] = HW_MODEL;
  size_t modellen = 80; // FIXME?
  fi->cpu_model = malloc(modellen);
  if(sysctl(mib, sizeof(mib) / sizeof(*mib), fi->cpu_model, &modellen, NULL, 0)){
    fprintf(stderr, "Coudln't acquire CTL_HW+HW_MODEL sysctl (%s)\n", strerror(errno));
    return -1;
  }
#endif
  return 0;
}

static int
fetch_cpu_info(fetched_info* fi){
  FILE* cpuinfo = fopen("/proc/cpuinfo", "re");
  if(cpuinfo == NULL){
    fprintf(stderr, "Error opening /proc/cpuinfo (%s)\n", strerror(errno));
    return -1;
  }
  char buf[BUFSIZ];
  while(fgets(buf, sizeof(buf), cpuinfo)){
#define CORE "core id"
// model name doesn't appear on all architectures, so fall back to vendor_id
#define TAG "model name"
#define VEND "vendor_id"
    if(strncmp(buf, TAG, strlen(TAG)) == 0){
      // model name trumps vendor_id
      char* start = strchr(buf + strlen(TAG), ':');
      if(start){
        ++start;
        char* nl = strchr(start, '\n');
        *nl = '\0';
        free(fi->cpu_model);
        fi->cpu_model = strdup(start);
      }
    }else if(strncmp(buf, VEND, strlen(VEND)) == 0){
      // vendor_id ought only be used in the absence of model name
      if(fi->cpu_model == NULL){
        char* start = strchr(buf + strlen(VEND), ':');
        if(start){
          ++start;
          char* nl = strchr(start, '\n');
          *nl = '\0';
          fi->cpu_model = strdup(start);
        }
      }
    }else if(strncmp(buf, CORE, strlen(CORE)) == 0){
      ++fi->core_count;
    }
#undef VEND
#undef TAG
#undef CORE
  }
  return 0;
}

static char*
pipe_getline(const char* cmdline){
  FILE* p = popen(cmdline, "re");
  if(p == NULL){
    fprintf(stderr, "Error running %s (%s)\n", cmdline, strerror(errno));
    return NULL;
  }
  char* buf = malloc(BUFSIZ); // gatesv("BUFSIZ bytes is enough for anyone")
  if(fgets(buf, BUFSIZ, p) == NULL){
//fprintf(stderr, "Error reading from %s (%s)\n", cmdline, strerror(errno));
    pclose(p);
    free(buf);
    return NULL;
  }
  // FIXME read any remaining junk so as to stave off SIGPIPEs?
  if(pclose(p)){
    fprintf(stderr, "Error closing pipe (%s)\n", strerror(errno));
    free(buf);
    return NULL;
  }
  return buf;
}

static int
fetch_x_props(fetched_info* fi){
  char* xrandr = pipe_getline("xrandr --current 2>/dev/null");
  if(xrandr == NULL){
    return -1;
  }
  char* randrcurrent = strstr(xrandr, " current ");
  if(randrcurrent == NULL){
    free(xrandr);
    return -1;
  }
  randrcurrent += strlen(" current ");
  if(sscanf(randrcurrent, "%d x %d", &fi->dimx, &fi->dimy) != 2){
    free(xrandr);
    return -1;
  }
  free(xrandr);
  return 0;
}

// Given a filename, check for its existence in the directories specified by
// https://specifications.freedesktop.org/icon-theme-spec/latest/ar01s03.html.
// Returns NULL if no such file can be found. Return value is heap-allocated.
static char *
get_xdg_logo(const char *spec){
  const char* logopath = "/usr/share/pixmaps/";
  int dfd = open(logopath, O_CLOEXEC | O_DIRECTORY);
  if(dfd < 0){
    return NULL;
  }
  int r = faccessat(dfd, spec, R_OK, 0);
  close(dfd);
  if(r){
    return NULL;
  }
  char* p = malloc(strlen(spec) + strlen(logopath) + 1);
  strcpy(p, logopath);
  strcat(p, spec);
  return p;
}

// FIXME deal more forgivingly with quotation marks
static const distro_info*
linux_ncneofetch(fetched_info* fi){
  FILE* osinfo = fopen("/etc/os-release", "re");
  if(osinfo == NULL){
    return NULL;
  }
  char buf[BUFSIZ];
  char* distro = NULL;
  while(fgets(buf, sizeof(buf), osinfo)){
#define PRETTY "PRETTY_NAME=\""
#define ID "ID=" // no quotes on this one
#define LOGOQ "LOGO=\""
#define LOGO "LOGO=" // handle LOGO sans quotes
    if(strncmp(buf, ID, strlen(ID)) == 0){
      char* nl = strchr(buf + strlen(ID), '\n');
      if(nl){
        *nl = '\0';
        distro = strdup(buf + strlen(ID));
      }
    }else if(!fi->distro_pretty && strncmp(buf, PRETTY, strlen(PRETTY)) == 0){
      char* nl = strchr(buf + strlen(PRETTY), '"');
      if(nl){
        *nl = '\0';
        fi->distro_pretty = strdup(buf + strlen(PRETTY));
      }
    }else if(!fi->logo && strncmp(buf, LOGOQ, strlen(LOGOQ)) == 0){
      char* nl = strchr(buf + strlen(LOGOQ), '"');
      if(nl){
        *nl = '\0';
        fi->logo = get_xdg_logo(buf + strlen(LOGOQ));
      }
    }else if(!fi->logo && strncmp(buf, LOGO, strlen(LOGO)) == 0){
      char* nl = strchr(buf + strlen(LOGO), '\n');
      if(nl){
        *nl = '\0';
        fi->logo = get_xdg_logo(buf + strlen(LOGO));
      }
    }
  }
#undef LOGO
#undef LOGOQ
#undef ID
#undef PRETTY
  fclose(osinfo);
  if(distro == NULL){
    return NULL;
  }
  const distro_info* dinfo = NULL;
  for(dinfo = distros ; dinfo->name ; ++dinfo){
    if(strcmp(dinfo->name, distro) == 0){
      break;
    }
  }
  if(fi->logo == NULL){
    fi->neologo = get_neofetch_art(distro);
  }
  free(distro);
  return dinfo;
}

typedef enum {
  NCNEO_LINUX,
  NCNEO_FREEBSD,
  NCNEO_DRAGONFLY,
  NCNEO_XNU,
  NCNEO_WINDOWS,
  NCNEO_UNKNOWN,
} ncneo_kernel_e;

static ncneo_kernel_e
get_kernel(fetched_info* fi){
#ifndef __MINGW64__
  struct utsname uts;
  if(uname(&uts)){
    fprintf(stderr, "Failure invoking uname (%s)\n", strerror(errno));
    return -1;
  }
  fi->kernel = strdup(uts.sysname);
  fi->kernver = strdup(uts.release);
  if(strcmp(uts.sysname, "Linux") == 0){
    return NCNEO_LINUX;
  }else if(strcmp(uts.sysname, "FreeBSD") == 0){
    return NCNEO_FREEBSD;
  }else if(strcmp(uts.sysname, "DragonFly") == 0){
    return NCNEO_DRAGONFLY;
  }else if(strcmp(uts.sysname, "Darwin") == 0){
    return NCNEO_XNU;
  }
  fprintf(stderr, "Unknown operating system via uname: %s\n", uts.sysname);
#else
  (void)fi;
  return NCNEO_WINDOWS;
#endif
  return NCNEO_UNKNOWN;
}

static const distro_info*
windows_ncneofetch(fetched_info* fi){
  static const distro_info mswin = {
    .name = "Windows",
    .logofile = NULL, // FIXME
  };
  fi->neologo = get_neofetch_art("Windows");
  fi->distro_pretty = NULL;
  return &mswin;
}

static const distro_info*
freebsd_ncneofetch(fetched_info* fi){
  static const distro_info fbsd = {
    .name = "FreeBSD",
    .logofile = NULL, // FIXME
  };
  fi->neologo = get_neofetch_art("BSD"); // use big daemon logo
  fi->distro_pretty = NULL;
  return &fbsd;
}

static const distro_info*
dragonfly_ncneofetch(fetched_info* fi){
  static distro_info fbsd = {
    .name = "DragonFly BSD",
    .logofile = NULL, // FIXME
  };
  fi->neologo = get_neofetch_art("dragonfly");
  fi->distro_pretty = NULL;
  return &fbsd;
}

static const distro_info*
xnu_ncneofetch(fetched_info* fi){
  static distro_info xnu = {
    .name = "OS X",
    .logofile = "/System/Library/PrivateFrameworks/LoginUIKit.framework/Versions/A/Frameworks/LoginUICore.framework/Versions/A/Resources/apple@2x.png",
  };
  fi->neologo = get_neofetch_art("Darwin");
  fi->distro_pretty = notcurses_osversion();
  return &xnu;
}

static int
drawpalette(struct notcurses* nc){
  int psize = notcurses_palette_size(nc);
  if(psize > 256){
    psize = 256;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  if(dimx < 64){
    return -1;
  }
  for(int y = 0 ; y < (psize + 63) / 64 ; ++y){
    // we show a maximum of 64 palette entries per line
    int toshow = psize - y * 64;
    if(toshow > 64){
      toshow = 64;
    }
    // center based on the number being shown on this line
    if(ncplane_cursor_move_yx(n, -1, (dimx - toshow) / 2)){
      return -1;
    }
    for(int x = (dimx - 64) / 2 ; x < dimx / 2 + 32 ; ++x){
      const int truex = x - (dimx - 64) / 2;
      if(y * 64 + truex >= psize){
        break;
      }
      if(ncplane_set_bg_palindex(n, y * 64 + truex)){
        return -1;
      }
      if(ncplane_putchar(n, ' ') == EOF){
        return -1;
      }
    }
    ncplane_set_bg_default(n);
    if(ncplane_putchar(n, '\n') == EOF){
      return -1;
    }
  }
  return 0;
}

static int
infoplane_notcurses(struct notcurses* nc, const fetched_info* fi, int planeheight){
  const int planewidth = 72;
  int dimy;
  int y;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, NULL);
  ncplane_cursor_yx(std, &y, NULL);
  struct ncplane_options nopts = {
    .y = y,
    .x = NCALIGN_CENTER,
    .rows = planeheight,
    .cols = planewidth,
    .userptr = NULL,
    .name = "info",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* infop = ncplane_create(std, &nopts);
  if(infop == NULL){
    return -1;
  }
  ncplane_set_fg_rgb8(infop, 0xd0, 0xd0, 0xd0);
  ncplane_set_styles(infop, NCSTYLE_UNDERLINE);
  ncplane_printf_aligned(infop, 1, NCALIGN_LEFT, " %s %s", fi->kernel, fi->kernver);
  if(fi->distro_pretty){
    ncplane_printf_aligned(infop, 1, NCALIGN_RIGHT, "%s ", fi->distro_pretty);
  }
  ncplane_set_styles(infop, NCSTYLE_BOLD);
#if defined(__linux__)
  struct sysinfo sinfo;
  sysinfo(&sinfo);
  char totalmet[BPREFIXSTRLEN + 1], usedmet[BPREFIXSTRLEN + 1];
  bprefix(sinfo.totalram, 1, totalmet, 1);
  bprefix(sinfo.totalram - sinfo.freeram, 1, usedmet, 1);
  ncplane_printf_aligned(infop, 2, NCALIGN_RIGHT, "Processes: %hu ", sinfo.procs);
  ncplane_printf_aligned(infop, 2, NCALIGN_LEFT, " RAM: %sB/%sB", usedmet, totalmet);
#elif defined(BSD)
  uint64_t ram;
  size_t oldlenp = sizeof(ram);
  if(sysctlbyname("hw.memsize", &ram, &oldlenp, NULL, 0) == 0){
    char tram[BPREFIXSTRLEN + 1];
    bprefix(ram, 1, tram, 1);
    ncplane_printf_aligned(infop, 2, NCALIGN_LEFT, " RAM: %sB", tram);
  }
#endif
  ncplane_printf_aligned(infop, 3, NCALIGN_LEFT, " DM: %s", fi->desktop ? fi->desktop : "n/a");
  ncplane_printf_aligned(infop, 3, NCALIGN_RIGHT, "Shell: %s ", fi->shell ? fi->shell : "n/a");
  if(notcurses_cantruecolor(nc)){
    ncplane_printf_aligned(infop, 4, NCALIGN_LEFT, " RGB TERM: %s", fi->term);
    nccell c = CELL_CHAR_INITIALIZER('R');
    nccell_set_styles(&c, NCSTYLE_BOLD);
    nccell_set_fg_rgb8(&c, 0xf0, 0xa0, 0xa0);
    ncplane_putc_yx(infop, 4, 1, &c);
    nccell_load_char(infop, &c, 'G');
    nccell_set_fg_rgb8(&c, 0xa0, 0xf0, 0xa0);
    ncplane_putc_yx(infop, 4, 2, &c);
    nccell_load_char(infop, &c, 'B');
    nccell_set_fg_rgb8(&c, 0xa0, 0xa0, 0xf0);
    ncplane_putc_yx(infop, 4, 3, &c);
    nccell_set_styles(&c, NCSTYLE_NONE);
  }else{
    ncplane_printf_aligned(infop, 4, NCALIGN_LEFT, " TERM: %s", fi->term);
  }
  ncplane_printf_aligned(infop, 4, NCALIGN_RIGHT, "Screen0: %dx%d ", fi->dimx, fi->dimy);
  ncplane_printf_aligned(infop, 5, NCALIGN_LEFT, " LANG: %s", fi->lang);
#ifndef __MINGW64__
  ncplane_printf_aligned(infop, 5, NCALIGN_RIGHT, "UID: %ju ", (uintmax_t)getuid());
#else
  ncplane_printf_aligned(infop, 5, NCALIGN_RIGHT, "UID: %s ", "FIXME"); // FIXME
#endif
  ncplane_set_styles(infop, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  ncplane_printf_aligned(infop, 6, NCALIGN_CENTER, "%s (%d cores)", fi->cpu_model, fi->core_count);
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(nccells_rounded_box(infop, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  nccell_set_fg_rgb8(&ul, 0x90, 0x90, 0x90);
  nccell_set_fg_rgb8(&ur, 0x90, 0x90, 0x90);
  nccell_set_fg_rgb8(&ll, 0, 0, 0);
  nccell_set_fg_rgb8(&lr, 0, 0, 0);
  unsigned ctrlword = NCBOXGRAD_BOTTOM | NCBOXGRAD_LEFT | NCBOXGRAD_RIGHT;
  if(ncplane_perimeter(infop, &ul, &ur, &ll, &lr, &hl, &vl, ctrlword)){
    return -1;
  }
  ncplane_home(infop);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0, 0xff, 0);
  ncplane_hline_interp(infop, &hl, planewidth / 2, ul.channels, channels);
  ncplane_hline_interp(infop, &hl, planewidth / 2, channels, ur.channels);
  nccell_release(infop, &ul); nccell_release(infop, &ur);
  nccell_release(infop, &ll); nccell_release(infop, &lr);
  nccell_release(infop, &hl); nccell_release(infop, &vl);
  ncplane_set_fg_rgb8(infop, 0xff, 0xff, 0xff);
  ncplane_set_styles(infop, NCSTYLE_BOLD);
  if(ncplane_printf_aligned(infop, 0, NCALIGN_CENTER, "[ %s@%s ]",
                            fi->username, fi->hostname) < 0){
    return -1;
  }
  ncchannels_set_fg_rgb8(&channels, 0, 0, 0);
  ncchannels_set_bg_rgb8(&channels, 0x50, 0x50, 0x50);
  ncplane_set_base(infop, " ", 0, channels);
  ncplane_scrollup_child(std, infop);
  if(notcurses_render(nc)){
    return -1;
  }
  return 0;
}

static int
infoplane(struct notcurses* nc, const fetched_info* fi){
  const int planeheight = 8;
  int r = infoplane_notcurses(nc, fi, planeheight);
  r |= notcurses_stop(nc);
  return r;
}

struct marshal {
  struct notcurses* nc;
  const distro_info* dinfo;
  const char* logo; // read from /etc/os-release (or builtin), may be NULL
  const char* neologo; // fallback from neofetch, text with color sub templates
};

// present a neofetch-style logo. we want to substitute colors for ${cN} inline
// sequences, and center the logo.
static int
neologo_present(struct notcurses* nc, const char* nlogo){
  // find the maximum line length in columns by iterating over the logo
  size_t maxlinelen = 0;
  size_t linelen; // length in bytes, including newline
  char** lines = NULL;
  int linecount = 0;
  for(const char* cur = nlogo ; *cur ; cur += linelen){
    const char* nl = strchr(cur, '\n');
    if(nl){
      linelen = (nl + 1) - cur;
    }else{
      linelen = strlen(cur);
    }
    char** tmpl;
    if((tmpl = realloc(lines, sizeof(*lines) * (linecount + 1))) == NULL){
      free(lines);
      return -1;
    }
    lines = tmpl;
    lines[linecount++] = strndup(cur, linelen);
    size_t collen = ncstrwidth(lines[linecount - 1]);
    if(collen > maxlinelen){
      maxlinelen = collen;
    }
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  const int leftpad = (dimx - maxlinelen) / 2;
  for(int i = 0 ; i < linecount ; ++i){
    ncplane_printf(n, "%*.*s%s", leftpad, leftpad, "", lines[i]);
    free(lines[i]);
  }
  free(lines);
  ncplane_set_fg_default(n);
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_ITALIC);
  if(notcurses_canopen_images(nc)){
    ncplane_putstr_aligned(n, -1, NCALIGN_CENTER, "(no image file is known for your distro)");
  }else{
    ncplane_putstr_aligned(n, -1, NCALIGN_CENTER, "(notcurses was compiled without image support)");
  }
  ncplane_off_styles(n, NCSTYLE_BOLD | NCSTYLE_ITALIC);
  return notcurses_render(nc);
}

static void*
display_thread(void* vmarshal){
  struct marshal* m = vmarshal;
  drawpalette(m->nc);
  notcurses_render(m->nc);
  ncplane_set_bg_default(notcurses_stdplane(m->nc));
  ncplane_set_fg_default(notcurses_stdplane(m->nc));
  // we've just rendered, so any necessary scrolling has been performed. draw
  // our image wherever the palette ended, and then scroll as necessary to
  // make that new plane visible.
  if(notcurses_canopen_images(m->nc)){
    struct ncvisual* ncv = NULL;
    if(m->logo){
      ncv = ncvisual_from_file(m->logo);
    }else if(m->dinfo && m->dinfo->logofile){
      ncv = ncvisual_from_file(m->dinfo->logofile);
    }
    if(ncv){
      int y;
      ncplane_cursor_yx(notcurses_stdplane_const(m->nc), &y, NULL);
      bool pixeling = false;
      if(notcurses_check_pixel_support(m->nc) >= 1){
        pixeling = true;
      }
      struct ncvisual_options vopts = {
        .n = notcurses_stdplane(m->nc),
        .x = NCALIGN_CENTER,
        .blitter = pixeling ? NCBLIT_PIXEL : NCBLIT_3x2,
        .scaling = pixeling ? NCSCALE_NONE : NCSCALE_SCALE_HIRES,
        .flags = NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_CHILDPLANE,
      };
      struct ncplane* iplane = ncvisual_blit(m->nc, ncv, &vopts);
      ncvisual_destroy(ncv);
      if(iplane){
        int x = ncplane_x(iplane);
        ncplane_move_yx(iplane, y, x);
        ncplane_scrollup_child(notcurses_stdplane(m->nc), iplane);
        notcurses_render(m->nc);
        ncplane_cursor_move_yx(notcurses_stdplane(m->nc),
                               ncplane_abs_y(iplane) + ncplane_dim_y(iplane), 0);
        return NULL;
      }
    }
  }
  if(m->neologo){
    if(neologo_present(m->nc, m->neologo) == 0){
      return NULL;
    }
  }
  return NULL;
}

static int
ncneofetch(struct notcurses* nc){
  fetched_info fi = {};
  ncneo_kernel_e kern = get_kernel(&fi);
  switch(kern){
    case NCNEO_LINUX:
      fi.distro = linux_ncneofetch(&fi);
      break;
    case NCNEO_FREEBSD:
      fi.distro = freebsd_ncneofetch(&fi);
      break;
    case NCNEO_DRAGONFLY:
      fi.distro = dragonfly_ncneofetch(&fi);
      break;
    case NCNEO_XNU:
      fi.distro = xnu_ncneofetch(&fi);
      break;
    case NCNEO_WINDOWS:
      fi.distro = windows_ncneofetch(&fi);
      break;
    case NCNEO_UNKNOWN:
      break;
  }
  // go ahead and spin the image load + render into its own thread while the
  // rest of the fetching continues. cuts total runtime.
  struct marshal display_marshal = {
    .nc = nc,
    .dinfo = fi.distro,
    .logo = fi.logo,
    .neologo = fi.neologo,
  };
  pthread_t tid;
  const bool launched = !pthread_create(&tid, NULL, display_thread, &display_marshal);
  fi.hostname = notcurses_hostname();
  fi.username = notcurses_accountname();
  fetch_env_vars(nc, &fi);
  if(kern != NCNEO_XNU && kern != NCNEO_WINDOWS){
    fetch_x_props(&fi);
  }
  if(kern == NCNEO_LINUX){
    fetch_cpu_info(&fi);
  }else{
    fetch_bsd_cpuinfo(&fi);
  }
  if(launched){
    pthread_join(tid, NULL);
  }
  if(infoplane(nc, &fi)){
    free_fetched_info(&fi);
    return -1;
  }
  free_fetched_info(&fi);
  return 0;
}

static void
usage(const char* arg0, FILE* fp){
  fprintf(fp, "usage: %s [ -v ]\n", arg0);
  if(fp == stderr){
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){
  struct notcurses_options opts = {
    .flags = NCOPTION_SUPPRESS_BANNERS
             | NCOPTION_NO_ALTERNATE_SCREEN
             | NCOPTION_NO_CLEAR_BITMAPS
             | NCOPTION_PRESERVE_CURSOR
             | NCOPTION_DRAIN_INPUT,
  };
  if(argc > 2){
    usage(argv[0], stderr);
  }else if(argc == 2){
    if(strcmp(argv[1], "-v") == 0){
      opts.loglevel = NCLOGLEVEL_TRACE;
    }else{
      usage(argv[0], stderr);
    }
  }
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_scrolling(stdn, true);
  int r = ncneofetch(nc);
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
