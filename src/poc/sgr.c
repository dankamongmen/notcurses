#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <notcurses.h>

static int
pivot_on(int pivot, int* sgrs, int sgrcount){
  assert(0 <= pivot);
  assert(sgrcount > pivot);
  int i;
  for(i = 0 ; i < sgrcount ; ++i){
    printf("%c", sgrs[i] ? '1' : '0');
  }
  putchar('\n');
  for(i = 8 ; i >= pivot ; --i){
    if(sgrs[i] == 0){
      sgrs[i] = 1;
      int j;
      for(j = i + 1 ; j < sgrcount ; ++j){
        sgrs[j] = 0;
      }
      return pivot;
    }
  }
  return pivot - 1;
}

int main(int argc, char** argv){
  setlocale(LC_ALL, NULL);
  const char* sgr;
  char** a;
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
  // generate all values
  while(pivot >= 0){
    pivot = pivot_on(pivot, sgrs, 9);
    int p = putp(tiparm(sgr, sgrs[0], sgrs[1], sgrs[2], sgrs[3], sgrs[4],
                             sgrs[5], sgrs[6], sgrs[7], sgrs[8]));
    assert(OK == p);
    for(a = argv ; *a ; ++a){
      if((p = printf("%s\n", *a)) < 0){
        return EXIT_FAILURE;
      }
    }
    p = putp(tiparm(sgr, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    assert(OK == p);
  }
  return EXIT_SUCCESS;
}
