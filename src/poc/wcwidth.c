#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  for(int i = 0 ; i < 128 ; ++i){
    wchar_t w = i;
    printf("width('%02x'): %d\t", i, wcwidth(w));
  }
  printf("\n");
  return EXIT_SUCCESS;
}
