#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

int main(int argc, char **argv){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  if(argc <= 1){
    for(int i = 0 ; i < 128 ; ++i){
      wchar_t w = i;
      int width = wcwidth(w);
      printf("0x%02x: %d%c\t", i, width, width < 0 ? '!' : ' ');
      if(i % 4 == 3){
        printf("\n");
      }
    }
    printf("\n");
    return EXIT_SUCCESS;
  }
  while(*++argv){
    const char* arg = *argv;
    int totalcols = 0;
    size_t totalb = 0;
    int i = 0;
    while(*arg){
      mbstate_t mbs = {};
      wchar_t w;
      size_t conv = mbrtowc(&w, arg, strlen(arg), &mbs);
      if(conv == (size_t)-1 || conv == (size_t)-2){
        fprintf(stderr, "Invalid UTF-8: %s\n", arg);
        return EXIT_FAILURE;
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
    }
    printf("\n total width: %d  total bytes: %zu\n\n", totalcols, totalb);
    // FIXME this will be broken if totalcols > screen width
    printf("%s\n", *argv);
    for(int z = 0 ; z < totalcols ; ++z){
      putchar('0' + z % 10);
    }
    putchar('\n');
    if(totalcols > 20){
      for(int z = 0 ; z < totalcols ; ++z){
        if(z % 10){
          putchar(' ');
        }else{
          putchar('0' + z / 10);
        }
      }
      putchar('\n');
    }
  }
  return EXIT_SUCCESS;
}
