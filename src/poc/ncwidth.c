#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <notcurses/direct.h>

static int
add_wchar(wchar_t** wbuf, size_t* bufsize, size_t* used, wchar_t wc){
  if(*used == *bufsize){
    const size_t GROW = 128;
    wchar_t* tmp = realloc(*wbuf, *bufsize + GROW * sizeof(**wbuf));
    if(tmp == NULL){
      return -1;
    }
    *wbuf = tmp;
    *bufsize += GROW * sizeof(**wbuf);
  }
  (*wbuf)[*used] = wc;
  ++*used;
  return 0;
}

static int
defaultout(void){
  for(int i = 0 ; i < 128 ; ++i){
    wchar_t w = i;
    int width = wcwidth(w);
    printf("0x%02x: %d%c\t", i, width, width < 0 ? '!' : ' ');
    if(i % 4 == 3){
      printf("\n");
    }
  }
  printf("\n");
  return 0;
}

int main(int argc, char **argv){
  if(argc <= 1){
    return defaultout() ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  struct ncdirect* n;
  if((n = ncdirect_core_init(NULL, NULL, 0)) == NULL){
    return EXIT_FAILURE;
  }
  size_t bufsize = 0, used = 0;
  wchar_t* wbuf = NULL;
  while(*++argv){
    const char* arg = *argv;
    int totalcols = 0;
    size_t totalb = 0;
    int i = 0;
    used = 0;
    while(*arg){
      mbstate_t mbs = {};
      wchar_t w;
      size_t conv = mbrtowc(&w, arg, strlen(arg), &mbs);
      if(conv == (size_t)-1 || conv == (size_t)-2){
        fprintf(stderr, "Invalid UTF-8: %s\n", arg);
        free(wbuf);
        goto err;
      }
      int width = wcwidth(w);
      printf("0x%05lx: %d %lc\t", (long)w, width, w);
      if(i++ % 4 == 3){
        printf("\n");
      }
      if(width > 0){
        totalcols += width;
      }
      arg += conv;
      totalb += conv;
      add_wchar(&wbuf, &bufsize, &used, w);
    }
    int y, x, newy, newx;
    putchar('\n');
    ncdirect_cursor_yx(n, &y, &x);
    printf("%s", *argv);
    fflush(stdout);
    ncdirect_cursor_yx(n, &newy, &newx);
    int realcols = (newx - x) + ncdirect_dim_x(n) * (newy - y);
    printf("\n iterated wcwidth: %d total bytes: %zu wcswidth: %d true width: %d\n\n",
           totalcols, totalb, wcswidth(wbuf, used), realcols);
    ncdirect_cursor_yx(n, &y, &x);
    // throw up a background color for invisible glyphs
    uint64_t chan = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0x80, 0);
    int expy, expx;
    int misses = 0;
    int scrolls = 0;
    while(**argv){
      int sbytes;
      int cols;
      if((cols = ncdirect_putegc(n, chan, *argv, &sbytes)) < 0){
        goto err;
      }
      fflush(stdout);
      ncdirect_cursor_yx(n, &newy, &newx);
      if(newy != y){
        newx += ncdirect_dim_x(n) * (newy - y);
      }
      ncdirect_cursor_push(n);
      if(x + cols != newx){
        ++misses;
        for(i = 0 ; i < misses ; ++i){
          putchar('\v');
        }
        printf("True width: %d wcwidth: %d [%.*s]", newx - x, cols, sbytes, *argv);
        ncdirect_cursor_yx(n, &expy, &expx);
        scrolls = (newy + misses) - expy;
        if(scrolls > 1){
          ncdirect_cursor_up(n, scrolls - 1);
        }
      }
      ncdirect_cursor_pop(n);
      *argv += sbytes;
      y = newy - (scrolls - 1);
      x = newx;
    }
    for(i = 0 ; i < misses + 1 ; ++i){
      putchar('\n');
    }
    ncdirect_set_fg_default(n);
    ncdirect_set_bg_default(n);
    for(int z = 0 ; z < realcols && z < ncdirect_dim_x(n) ; ++z){
      putchar('0' + z % 10);
    }
    if(realcols < ncdirect_dim_x(n)){
      putchar('\n');
    }
    if(realcols > 20){
      for(int z = 0 ; z < realcols && z < ncdirect_dim_x(n) ; ++z){
        if(z % 10){
          putchar(' ');
        }else{
          putchar('0' + z / 10);
        }
      }
      putchar('\n');
    }
  }
  free(wbuf);
  return ncdirect_stop(n) ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  ncdirect_stop(n);
  return EXIT_FAILURE;
}
