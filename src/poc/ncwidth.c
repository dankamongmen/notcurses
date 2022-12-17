#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <compat/compat.h>
#include <notcurses/direct.h>

static int
add_wchar(wchar_t** wbuf, size_t* bufsize, size_t* used, wchar_t wc){
  if(*used == *bufsize){
    const size_t GROW = 128;
    wchar_t* tmp = realloc(*wbuf, (*bufsize + GROW) * sizeof(**wbuf));
    if(tmp == NULL){
      return -1;
    }
    *wbuf = tmp;
    *bufsize += GROW;
  }
  (*wbuf)[*used] = wc;
  ++*used;
  return 0;
}

static int
defaultout(void){
  for(int i = 0 ; i < 128 ; ++i){
    wint_t w = i;
    int width = wcwidth(w);
    if(printf("0x%02x: %d%c\t", w, width, width < 0 ? '!' : ' ') < 0){
      return -1;
    }
    if(i % 4 == 3){
      if(printf("\n") < 0){
        return -1;
      }
    }
  }
  if(printf("\n") < 0){
    return -1;
  }
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
      mbstate_t mbs = {0};
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
    putchar('\n');
    unsigned y, x;
    ncdirect_cursor_yx(n, &y, &x);
    printf("%s", *argv);
    fflush(stdout);
    unsigned newy, newx;
    ncdirect_cursor_yx(n, &newy, &newx);
    unsigned realcols = (newx - x) + ncdirect_dim_x(n) * (newy - y);
    printf("\n iterated wcwidth: %d total bytes: %llu wcswidth: %d true width: %d\n\n",
           totalcols, (unsigned long long)totalb, wcswidth(wbuf, used), realcols);
    ncdirect_cursor_yx(n, &y, &x);
    // throw up a background color for invisible glyphs
    uint64_t chan = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0x80, 0);
    int scrolls = 0, newscrolls = 0;
    while(**argv){
      int sbytes;
      int cols;
      if((cols = ncdirect_putegc(n, chan, *argv, &sbytes)) < 0){
        goto err;
      }
      fflush(stdout);
      ncdirect_cursor_yx(n, &newy, &newx);
//fprintf(stderr, "NE: %d %d %d %d\n", newy, newx, y, x);
      if(newy != y){
        newx += ncdirect_dim_x(n) * (newy - y);
      }
      if(x + cols != newx){
        newscrolls = 0;
        ++scrolls;
        for(int k = 0 ; k < scrolls ; ++k){
          if(newy >= ncdirect_dim_y(n)){
            ++newscrolls;
            putchar('\v');
          }else{
            ncdirect_cursor_down(n, 1);
            ++newy;
          }
        }
        printf("True width: %d wcwidth: %d [%.*s]", newx - x, cols, sbytes, *argv);
        ncdirect_cursor_move_yx(n, newy - newscrolls, newx);
      }
      *argv += sbytes;
      y = newy + newscrolls;
      x = newx;
    }
    for(i = 0 ; i < scrolls + 1 ; ++i){
      putchar('\n');
    }
    ncdirect_set_fg_default(n);
    ncdirect_set_bg_default(n);
    for(unsigned z = 0 ; z < realcols && z < ncdirect_dim_x(n) ; ++z){
      putchar('0' + z % 10);
    }
    if(realcols < ncdirect_dim_x(n)){
      putchar('\n');
    }
    if(realcols > 20){
      for(unsigned z = 0 ; z < realcols && z < ncdirect_dim_x(n) ; ++z){
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
