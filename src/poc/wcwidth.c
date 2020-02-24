#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  for(int i = 0 ; i < 128 ; ++i){
    wchar_t w = i;
    printf("w('%02x'): %d%c\t", i, wcwidth(w), iscntrl(i) ? '!' : ' ');
    if(i % 4 == 3){
      printf("\n");
    }
  }
  printf("\n");
  return EXIT_SUCCESS;
}
