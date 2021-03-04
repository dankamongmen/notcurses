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
#include <filesystem>
#include <sys/types.h>
#include <ncpp/Direct.hh>
#ifndef __linux__
#define AT_NO_AUTOMOUNT 0 // not defined on freebsd
#endif

void usage(std::ostream& os, const char* name, int code){
  os << "usage: " << name << " -h | -V | [ -lLR ] [ --align type ] paths...\n";
  os << " -d: list directories themselves, not their contents\n";
  os << " -l: use a long listing format\n";
  os << " -L: dereference symlink arguments\n";
  os << " -R: list subdirectories recursively\n";
  os << " -a|--align type: one of left, right, or center\n";
  os << " -h: print usage information\n";
  os << " -V: print version information\n";
  os << std::flush;
  exit(code);
}

struct job {
  std::filesystem::path dir;
  std::string p;
};

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t outmtx = PTHREAD_MUTEX_INITIALIZER; // guards standard out
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::queue<job> work;  // jobs available for workers
bool keep_working;     // set false when we're done so threads die

// context as configured on the command line
struct lsContext {
  ncpp::Direct nc;
  bool longlisting;
  bool recursedirs;
  bool directories;
  bool dereflinks;
  ncalign_e alignment;
};

int handle_path(int dirfd, std::filesystem::path& dir, const char* p, const lsContext& ctx, bool toplevel);

// handle a single inode of arbitrary type
int handle_inode(std::filesystem::path& dir, const char* p, const struct stat* st, const lsContext& ctx){
  (void)st; // FIXME handle symlink (dereflinks)
  (void)ctx; // FIXME handle symlink (dereflinks)
  pthread_mutex_lock(&mtx);
  work.emplace(job{dir, p});
  pthread_mutex_unlock(&mtx);
  pthread_cond_signal(&cond);
  return 0;
}

// if |directories| is true, only print details of |p|, and return. otherwise,
// if |recursedirs| or |toplevel| is set, we will recurse, passing false as
// toplevel (but preserving |recursedirs|).
int handle_dir(int dirfd, std::filesystem::path& pdir, const char* p, const struct stat* st, const lsContext& ctx, bool toplevel){
  if(ctx.directories){
    return handle_inode(pdir, p, st, ctx);
  }
  if(!ctx.recursedirs && !toplevel){
    return handle_inode(pdir, p, st, ctx);
  }
  if((strcmp(p, ".") == 0 || strcmp(p, "..") == 0) && !toplevel){
    return 0;
  }
  int newdir = openat(dirfd, p, O_DIRECTORY | O_CLOEXEC);
  if(newdir < 0){
    std::cerr << "Error opening " << p << ": " << strerror(errno) << std::endl;
    return -1;
  }
  DIR* dir = fdopendir(newdir);
  auto subdir = pdir / p;
  if(dir == nullptr){
    std::cerr << "Error opening " << p << ": " << strerror(errno) << std::endl;
    close(newdir);
    return -1;
  }
  struct dirent* dent;
  int r = 0;
  while(errno = 0, (dent = readdir(dir))){
    r |= handle_path(newdir, subdir, dent->d_name, ctx, false);
  }
  if(errno){
    std::cerr << "Error reading from " << p << ": " << strerror(errno) << std::endl;
    closedir(dir);
    close(newdir);
    return -1;
  }
  closedir(dir);
  close(newdir);
  return 0;
}

int handle_deref(const char* p, const struct stat* st, const lsContext& ctx){
  (void)p;
  (void)st;
  (void)ctx; // FIXME dereference and rerun on target
  return 0;
}

// handle some path |p|, either absolute or relative to |dirfd|. |toplevel| is
// true iff the path was directly listed on the command line.
int handle_path(int dirfd, std::filesystem::path& pdir, const char* p, const lsContext& ctx, bool toplevel){
  struct stat st;
  if(fstatat(dirfd, p, &st, AT_NO_AUTOMOUNT)){
    std::cerr << "Error running fstatat(" << p << "): " << strerror(errno) << std::endl;
    return -1;
  }
  if((st.st_mode & S_IFMT) == S_IFDIR){
    return handle_dir(dirfd, pdir, p, &st, ctx, toplevel);
  }else if((st.st_mode & S_IFMT) == S_IFLNK){
    if(toplevel && ctx.dereflinks){
      return handle_deref(p, &st, ctx);
    }
  }
  return handle_inode(pdir, p, &st, ctx);
}

// return long-term return code
void ncls_thread(const lsContext* ctx) {
  while(true){
    pthread_mutex_lock(&mtx);
    while(work.empty() && keep_working){
      pthread_cond_wait(&cond, &mtx);
    }
    if(!work.empty()){
      job j = work.front();
      work.pop();
      pthread_mutex_unlock(&mtx);
      auto s = j.dir / j.p;
      auto faken = ctx->nc.prep_image(s.c_str(), NCBLIT_PIXEL, NCSCALE_SCALE_HIRES);
      pthread_mutex_lock(&outmtx);
      std::cout << j.p << '\n';
      if(faken){
        ctx->nc.raster_image(faken, ctx->alignment);
      }
      pthread_mutex_unlock(&outmtx);
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
    std::filesystem::path s;
    ret |= handle_path(dirfd, s, *argv, ctx, true);
    ++argv;
  }
  close(dirfd);
  return ret;
}

int main(int argc, char* const * argv){
  bool directories = false;
  bool recursedirs = false;
  bool longlisting = false;
  bool dereflinks = false;
  ncalign_e alignment = NCALIGN_RIGHT;
  const struct option opts[] = {
    { "align", 1, nullptr, 'a' },
    { "help", 0, nullptr, 'h' },
    { "version", 0, nullptr, 'V' },
    { nullptr, 0, nullptr, 0 },
  };
  int c, lidx;
  while((c = getopt_long(argc, argv, "Va:dhlLR", opts, &lidx)) != -1){
    switch(c){
      case 'V':
        printf("ncls version %s\n", notcurses_version());
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
  notcurses_check_
  auto procs = std::thread::hardware_concurrency();
  if(procs <= 0){
    procs = 4;
  }
  if(procs > 8){
    procs = 8;
  }
  std::vector<std::thread> threads;
  lsContext ctx = {
    ncpp::Direct(),
    longlisting,
    recursedirs,
    directories,
    dereflinks,
    alignment,
  };
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
  return EXIT_SUCCESS;
}
