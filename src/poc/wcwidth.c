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
  if(argc > 1){
    while(*++argv){
      const char* arg = *argv;
      while(*arg){
        mbstate_t mbs = {};
        wchar_t w;
        size_t conv = mbrtowc(&w, arg, strlen(arg), &mbs);
        if(conv == (size_t)-1 || conv == (size_t)-2){
          fprintf(stderr, "Invalid UTF-8: %s\n", arg);
          return EXIT_FAILURE;
        }
        int width = wcwidth(w);
        printf("w(0x%05lx): %d %lc\n", (long)w, width, w);
        arg += conv;
      }
    }
    return EXIT_SUCCESS;
  }
  for(int i = 0 ; i < 128 ; ++i){
    wchar_t w = i;
    printf("w(0x%02x): %d%c\t", i, wcwidth(w), iscntrl(i) ? '!' : ' ');
    if(i % 4 == 3){
      printf("\n");
    }
  }
  printf("\n");
  return EXIT_SUCCESS;
}
