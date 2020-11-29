#include <cstdlib>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ncpp/Direct.hh>

static void
usage(std::ostream& os, const char* name, int code){
  os << "usage: " << name << " -h | [ -lR ] paths...\n";
  os << " -d: list directories themselves, not their contents\n";
  os << " -l: use a long listing format\n";
  os << " -L: dereference symlink arguments\n";
  os << " -R: list subdirectories recursively\n";
  os << " -h: print usage information\n";
  os << std::flush;
  exit(code);
}

// context as configured on the command line
struct lsContext {
  ncpp::Direct nc;
  bool longlisting;
  bool recursedirs;
  bool directories;
  bool dereflinks;
};

static int
handle_path(const char* p, const lsContext& ctx, bool toplevel);

// handle a single inode of arbitrary type
static int
handle_inode(const char* p, const struct stat* st, const lsContext& ctx){
  std::cout << p << std::endl; // FIXME handle symlink (dereflinks)
  ctx.nc.render_image(p, NCALIGN_RIGHT, NCBLIT_DEFAULT, NCSCALE_STRETCH);
  return 0;
}

// if |directories| is true, only print details of |p|, and return. otherwise,
// if |recursedirs| or |toplevel| is set, we will recurse, passing false as
// toplevel (but preserving |recursedirs|).
static int
handle_dir(const char* p, const struct stat* st, const lsContext& ctx, bool toplevel){
  if(ctx.directories){
    return handle_inode(p, st, ctx);
  }
  if(!ctx.recursedirs && !toplevel){
    return 0;
  }
  DIR* dir = opendir(p);
  if(dir == NULL){
    std::cerr << "Error opening " << p << ": " << strerror(errno) << std::endl;
    return -1;
  }
  struct dirent* dent;
  int r = 0;
  while(errno = 0, (dent = readdir(dir))){
    r |= handle_path(dent->d_name, ctx, false);
  }
  if(errno){
    std::cerr << "Error reading from " << p << ": " << strerror(errno) << std::endl;
    closedir(dir);
    return -1;
  }
  closedir(dir);
  return 0;
}

static int
handle_deref(const char* p, const struct stat* st, const lsContext& ctx){
  // FIXME dereference and rerun on target
  return 0;
}

// handle some path, either absolute or relative to the current directory.
// toplevel is true iff the path was directly listed on the command line.
// recursedirs, directories, longlisting, and dereflinks are all based off
// command-line parameters.
static int
handle_path(const char* p, const lsContext& ctx, bool toplevel){
  struct stat st;
  if(stat(p, &st)){
    std::cerr << "Error running stat(" << p << "): " << strerror(errno) << std::endl;
    return -1;
  }
  if((st.st_mode & S_IFMT) == S_IFDIR){
    return handle_dir(p, &st, ctx, toplevel);
  }else if((st.st_mode & S_IFMT) == S_IFLNK){
    if(toplevel && ctx.dereflinks){
      return handle_deref(p, &st, ctx);
    }
  }
  return handle_inode(p, &st, ctx);
}

// these are our command line arguments. they're the only paths for which
// handle_path() gets toplevel == true.
static int
list_paths(const char* const * argv, const lsContext& ctx){
  int ret = 0;
  while(*argv){
    ret |= handle_path(*argv, ctx, true);
    ++argv;
  }
  return ret;
}

int main(int argc, char* const * argv){
  lsContext ctx = {
    .nc = ncpp::Direct(),
    .longlisting = false,
    .recursedirs = false,
    .directories = false,
    .dereflinks = false,
  };
  int c;
  while((c = getopt(argc, argv, "dhlLR")) != -1){
    switch(c){
      case 'd':
        ctx.directories = true;
        break;
      case 'l':
        ctx.longlisting = true;
        break;
      case 'L':
        ctx.dereflinks = true;
        break;
      case 'R':
        ctx.recursedirs = true;
        break;
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      default:
        usage(std::cerr, argv[0], EXIT_FAILURE);
        break;
    }
  }
  static const char* const default_args[] = { ".", nullptr };
  list_paths(argv[optind] ? argv + optind : default_args, ctx);
  return EXIT_SUCCESS;
}
