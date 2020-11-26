#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <notcurses/notcurses.h>

#define DISABLE_ALTCHARSET 1

static int
pivot_on(int pivot, int* sgrs, int sgrcount){
  assert(0 <= pivot);
  assert(sgrcount > pivot);
  int i;
  for(i = sgrcount - 1 ; i >= pivot ; --i){
    if(sgrs[i] == 0){
      sgrs[i] = 1;
      int j;
      for(j = i + 1 ; j < sgrcount ; ++j){
        sgrs[j] = 0;
      }
      return pivot;
    }
  }
  for(i = 8 ; i >= pivot ; --i){
    sgrs[i] = 0;
  }
  sgrs[i] = 1;
  return i;
}

int main(int argc, char** argv){
  (void)argc;
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  const char* sgr;
  if(setupterm(NULL, -1, NULL)){
    fprintf(stderr, "Error initializing terminal\n");
    return EXIT_FAILURE;
  }
  if((sgr = tigetstr("sgr")) == NULL || sgr == (char*)-1){
    fprintf(stderr, "Couldn't get terminfo entry for sgr\n");
    return EXIT_FAILURE;
  }
  int sgrs[9] = { 0 };
  int pivot = 8;
  if(DISABLE_ALTCHARSET){
    --pivot;
  }
  int sgrcount = pivot + 1;
  // generate all values, like a beast
  int cols = 0;
  while(pivot >= 0){
    int i;
    for(i = 0 ; i < 9 ; ++i){
      cols += printf("%c", sgrs[i] ? '1' : '0');
    }
    cols += printf(" (%02d)", pivot);
    i = putp(tiparm(sgr, sgrs[0], sgrs[1], sgrs[2], sgrs[3], sgrs[4],
                         sgrs[5], sgrs[6], sgrs[7], sgrs[8]));
    if(i != OK){
      return EXIT_FAILURE;
    }
    if((i = printf(" %s ", argv[0])) < 0){
      return EXIT_FAILURE;
    }
    cols += i;
    i = putp(tiparm(sgr, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    if(i != OK){
      return EXIT_FAILURE;
    }
    pivot = pivot_on(pivot, sgrs, sgrcount);
    if(cols >= 60){ // FIXME
      printf("\n");
      cols = 0;
    }else{
      cols += printf("  ");
    }
  }
  printf("\n");
  return EXIT_SUCCESS;
}
