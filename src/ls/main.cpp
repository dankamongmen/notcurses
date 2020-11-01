#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncpp/NotCurses.hh>

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

// handle a single inode of arbitrary type
static int
handle_inode(const char* p, const struct stat* st, bool longlisting){
  return 0;
}

// if |directories| is true, only print details of |p|, and return. otherwise,
// if |recursedirs| or |toplevel| is set, we will recurse, passing false as
// toplevel (but preserving |recursedirs|).
static int
handle_dir(const char* p, const struct stat* st, bool longlisting,
           bool recursedirs, bool directories, bool toplevel){
  if(directories){
    return handle_inode(p, st, longlisting);
  }
  std::cout << "DIRECTORY: " << p << std::endl; // FIXME handle directory (recursedirs, directories)
  return 0;
}

// handle a directory path *listed on the command line*.
static inline int
handle_cmdline_dir(const char* p, const struct stat* st, bool longlisting,
                   bool recursedirs, bool directories){
  return handle_dir(p, st, longlisting, recursedirs, directories, true);
}

static int
handle_path(const char* p, bool longlisting, bool recursedirs,
            bool directories, bool dereflinks){
  struct stat st;
  if(stat(p, &st)){
    std::cerr << "Error running stat(" << p << "): " << strerror(errno) << std::endl;
    return -1;
  }
  if((st.st_mode & S_IFMT) == S_IFDIR){
    return handle_cmdline_dir(p, &st, longlisting, recursedirs, directories);
  }else if((st.st_mode & S_IFMT) == S_IFLNK){
    std::cout << p << std::endl; // FIXME handle symlink (dereflinks)
  }else if((st.st_mode & S_IFMT) == S_IFREG){
    std::cout << p << std::endl; // FIXME handle normal file
  }else{
    // FIXME handle weirdo
  }
  return 0;
}

static int
list_paths(const char* const * argv, bool longlisting, bool recursedirs,
           bool directories, bool dereflinks){
  int ret = 0;
  while(*argv){
    ret |= handle_path(*argv, longlisting, recursedirs, directories, dereflinks);
    ++argv;
  }
  return ret;
}

int main(int argc, char* const * argv){
  bool longlisting = false;
  bool recursedirs = false;
  bool directories = false;
  bool dereflinks = false;
  int c;
  while((c = getopt(argc, argv, "dhlLR")) != -1){
    switch(c){
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
  static const char* const default_args[] = { ".", nullptr };
  list_paths(argv[optind] ? argv + optind : default_args, longlisting,
             recursedirs, directories, dereflinks);
  return EXIT_SUCCESS;
}
