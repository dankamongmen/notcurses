#define NCPP_EXCEPTIONS_PLEASE
#include <queue>
#include <thread>
#include <vector>
#include <atomic>
#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <compat/compat.h>
#include <notcurses/notcurses.h>
#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT 0  // not defined on freebsd and some older linux kernels
#endif

void usage(std::ostream& os, const char* name, int code){
  os << "usage: " << name << " -h | -V | [ -lLR ] [ --align type ] paths...\n";
  os << " -d: list directories themselves, not their contents\n";
  os << " -l: use a long listing format\n";
  os << " -L: dereference symlink arguments\n";
  os << " -R: list subdirectories recursively\n";
  os << " -a|--align type: 'left', 'right', or 'center'\n";
  os << " -b|--blitter blitter: 'ascii', 'half', 'quad', 'sex', 'braille', or 'pixel'\n";
  os << " -s|--scale scaling: one of 'none', 'hires', 'scale', 'scalehi', or 'stretch'\n";
  os << " -h: print usage information\n";
  os << " -V: print version information\n";
  os << std::flush;
  exit(code);
}

struct job {
  // FIXME ought be a const std::string, but Mojave and maybe other
  // platforms (any prior to C++17 for sure) are lamely lacking
  // std::filesystem. alas.
  // FIXME ideally we'd be handing off a dirfd, not a path.
  std::string dir;
  std::string p;
};

// FIXME see above; we ought have std::filesystem
auto path_join(const std::string& dir, const std::string& p) -> std::string {
  if(dir.empty()){
    return p;
  }
  return dir + path_separator() + p;
}

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t outmtx = PTHREAD_MUTEX_INITIALIZER; // guards standard out
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::queue<job> work;  // jobs available for workers
bool keep_working;     // set false when we're done so threads die

// context as configured on the command line
struct lsContext {
  struct notcurses *nc;
  bool longlisting;
  bool recursedirs;
  bool directories;
  bool dereflinks;
  // if we're using default scaling, we try to use NCSCALE_NONE, but if we're
  // too large for the visual area, we'll instead use NCSCALE_SCALE_HIRES. if
  // a scaling is specified, we use that no matter what.
  bool default_scaling;
  ncalign_e alignment;
  ncblitter_e blitter;
  ncscale_e scaling;
};

int handle_path(int dirfd, const std::string& dir, const char* p, const lsContext& ctx, bool toplevel);

// handle a single inode of arbitrary type
int handle_inode(const std::string& dir, const char* p){
  pthread_mutex_lock(&mtx);
  work.emplace(job{dir, p});
  pthread_mutex_unlock(&mtx);
  pthread_cond_signal(&cond);
  return 0;
}

// if |ctx->directories| is true, only print details of |p|, and return.
// otherwise, if |ctx->recursedirs| or |toplevel| is set, we will recurse,
// passing false for toplevel (but preserving |ctx|).
int handle_dir(int dirfd, const std::string& pdir, const char* p,
               const lsContext& ctx, bool toplevel){
  if(ctx.directories){
    return handle_inode(pdir, p);
  }
  if(!ctx.recursedirs && !toplevel){
    return handle_inode(pdir, p);
  }
  if((strcmp(p, ".") == 0 || strcmp(p, "..") == 0) && !toplevel){
    return 0;
  }
  int newdir = -1;
#ifndef __MINGW32__
  newdir = openat(dirfd, p, O_DIRECTORY | O_CLOEXEC);
  if(newdir < 0){
    std::cerr << "Error opening " << p << ": " << strerror(errno) << std::endl;
    return -1;
  }
  DIR* dir = fdopendir(newdir);
#else
  (void)dirfd;
  DIR* dir = opendir(path_join(pdir, p).c_str());
#endif
  if(dir == nullptr){
    std::cerr << "Error opening " << p << ": " << strerror(errno) << std::endl;
    close(newdir);
    return -1;
  }
  struct dirent* dent;
  int r = 0;
  while(errno = 0, (dent = readdir(dir))){
    r |= handle_path(newdir, path_join(pdir, p), dent->d_name, ctx, false);
  }
  if(errno){
    std::cerr << "Error reading from " << p << ": " << strerror(errno) << std::endl;
    closedir(dir);
    close(newdir);
    return -1;
  }
  closedir(dir);
  if(newdir >= 0){
    close(newdir);
  }
  return r;
}

// handle some path |p|, either absolute or relative to |dirfd|. |toplevel| is
// true iff the path was directly listed on the command line. we rely on lstat()
// and fstatat() to resolve symbolic links for us.
int handle_path(int dirfd, const std::string& pdir, const char* p, const lsContext& ctx, bool toplevel){
  struct stat st;
#ifndef __MINGW32__
  int flags = AT_NO_AUTOMOUNT;
  if(!ctx.dereflinks){
    flags |= AT_SYMLINK_NOFOLLOW;
  }
  if(fstatat(dirfd, p, &st, flags)){
    std::cerr << "Error running fstatat(" << p << "): " << strerror(errno) << std::endl;
    return -1;
  }
#else
  if(stat(path_join(pdir, p).c_str(), &st)){
    std::cerr << "Error running stat(" << p << "): " << strerror(errno) << std::endl;
    return -1;
  }
#endif
  if((st.st_mode & S_IFMT) == S_IFDIR){
    return handle_dir(dirfd, pdir, p, ctx, toplevel);
  }else if((st.st_mode & S_IFMT) == S_IFLNK){
    pthread_mutex_lock(&outmtx);
    std::cout << path_join(pdir, p) << '\n';
    pthread_mutex_unlock(&outmtx);
    return 0;
  }
  return handle_inode(pdir, p);
}

// return long-term return code
void ncls_thread(const lsContext* ctx) {
  unsigned dimy, dimx;
  ncplane* stdn = notcurses_stddim_yx(ctx->nc, &dimy, &dimx);
  while(true){
    pthread_mutex_lock(&mtx);
    while(work.empty() && keep_working){
      pthread_cond_wait(&cond, &mtx);
    }
    if(!work.empty()){
      job j = work.front();
      work.pop();
      pthread_mutex_unlock(&mtx);
      auto s = path_join(j.dir, j.p);
      auto ncv = ncvisual_from_file(s.c_str());
      struct ncplane* ncp = nullptr;
      if(ncv){
        struct ncvisual_options vopts{};
        vopts.blitter = ctx->blitter;
        vopts.scaling = ctx->scaling;
        if(ctx->default_scaling){
          struct ncvgeom geom;
          ncvisual_geom(ctx->nc, ncv, &vopts, &geom);
          if(geom.rcellx > dimx || geom.rcelly > dimy){
            vopts.scaling = NCSCALE_SCALE_HIRES;
          }
        }
        ncp = ncvisual_blit(ctx->nc, ncv, &vopts);
      }
      ncvisual_destroy(ncv);
      pthread_mutex_lock(&outmtx);
      ncplane_printf(stdn, "%s\n", j.p.c_str());
      if(ncp){
        ncplane_reparent(ncp, stdn);
        ncplane_move_yx(ncp, ncplane_cursor_y(stdn), ncplane_cursor_x(stdn));
        ncplane_scrollup_child(stdn, ncp);
        notcurses_render(ctx->nc);
        ncplane_cursor_move_yx(stdn, ncplane_dim_y(ncp) + ncplane_y(ncp), 0);
        ncplane_putchar(stdn, '\n');
        notcurses_render(ctx->nc);
      }
      pthread_mutex_unlock(&outmtx);
      // FIXME don't delete plane right now, or we wipe it, but we can't keep
      // them all open forevermore! free *any we have scrolled off*.
    }else if(!keep_working){
      pthread_mutex_unlock(&mtx);
      return;
    }
  }
}

// these are our command line arguments. they're the only paths for which
// handle_path() gets toplevel == true.
int list_paths(const char* const * argv, const lsContext& ctx){
  int dirfd = open(".", O_DIRECTORY | O_CLOEXEC);
  if(dirfd < 0){
    std::cerr << "Error opening current directory: " << strerror(errno) << std::endl;
    return -1;
  }
  int ret = 0;
  while(*argv){
    ret |= handle_path(dirfd, "", *argv, ctx, true);
    ++argv;
  }
  close(dirfd);
  return ret;
}

int main(int argc, char* const * argv){
  bool default_scaling = true;
  bool directories = false;
  bool recursedirs = false;
  bool longlisting = false;
  bool dereflinks = false;
  ncalign_e alignment = NCALIGN_RIGHT;
  ncblitter_e blitter = NCBLIT_PIXEL;
  ncscale_e scale = NCSCALE_NONE;
  const struct option opts[] = {
    { "align", 1, nullptr, 'a' },
    { "blitter", 1, nullptr, 'b' },
    { "scale", 1, nullptr, 's' },
    { "help", 0, nullptr, 'h' },
    { "version", 0, nullptr, 'V' },
    { nullptr, 0, nullptr, 0 },
  };
  int c, lidx;
  while((c = getopt_long(argc, argv, "Va:b:s:dhlLR", opts, &lidx)) != -1){
    switch(c){
      case 'V':
        std::cout << "ncls version " <<  notcurses_version() << std::endl;
        exit(EXIT_SUCCESS);
      case 'a':
        if(strcasecmp(optarg, "left") == 0){
          alignment = NCALIGN_LEFT;
        }else if(strcasecmp(optarg, "right") == 0){
          alignment = NCALIGN_RIGHT;
        }else if(strcasecmp(optarg, "center") == 0){
          alignment = NCALIGN_CENTER;
        }else{
          std::cerr << "Unknown alignment type: " << optarg << "\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 'b':
        if(notcurses_lex_blitter(optarg, &blitter)){
          std::cerr << "Invalid blitter specification (got "
                    << optarg << ")" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 's':
        if(notcurses_lex_scalemode(optarg, &scale)){
          std::cerr << "Invalid scaling specification (got "
                    << optarg << ")" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        default_scaling = false;
        break;
      case 'd':
        directories = true;
        break;
      case 'l':
        longlisting = true;
        break;
      case 'L':
        dereflinks = true;
        break;
      case 'R':
        recursedirs = true;
        break;
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      default:
        usage(std::cerr, argv[0], EXIT_FAILURE);
        break;
    }
  }
  auto procs = std::thread::hardware_concurrency();
  if(procs <= 0){
    procs = 4;
  }
  if(procs > 8){
    procs = 8;
  }
  std::vector<std::thread> threads;
  struct notcurses_options nopts{};
  nopts.flags |= NCOPTION_CLI_MODE
                | NCOPTION_SUPPRESS_BANNERS
                | NCOPTION_DRAIN_INPUT;
  lsContext ctx = {
    nullptr,
    longlisting,
    recursedirs,
    directories,
    dereflinks,
    default_scaling,
    alignment,
    blitter,
    scale,
  };
  if((ctx.nc = notcurses_init(&nopts, nullptr)) == nullptr){
    return EXIT_FAILURE;
  }
  keep_working = true;
  for(auto s = 0u ; s < procs ; ++s){
    threads.emplace_back(std::thread(ncls_thread, &ctx));
  }
  static const char* const default_args[] = { ".", nullptr };
  list_paths(argv[optind] ? argv + optind : default_args, ctx);
  keep_working = false;
  pthread_cond_broadcast(&cond);
  for(auto &t : threads){
//std::cerr << "Waiting on thread " << procs << std::endl;
    t.join();
    --procs;
  }
  return notcurses_stop(ctx.nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
