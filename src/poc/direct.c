#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <notcurses/notcurses.h>

// can we leave what was already on the screen there? (narrator: it seems not)
int main(void){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  struct winsize geom;
  if(ioctl(fileno(stdout), TIOCGWINSZ, &geom)){
    fprintf(stderr, "Couldn't get terminal geometry (%s)\n", strerror(errno));
    return EXIT_FAILURE;
  }
  for(int y = 0 ; y < geom.ws_row ; ++y){
    for(int x = 0 ; x < geom.ws_col ; ++x){
      printf("X");
    }
  }
  fflush(stdout);
  struct ncdirect* n; // see bug #391
  if((n = ncdirect_init(NULL, stdout)) == NULL){
    return EXIT_FAILURE;
  }
  int ret = 0;
  ret |= ncdirect_fg(n, 0xff8080);
  ret |= ncdirect_styles_on(n, NCSTYLE_STANDOUT);
  printf(" erp erp \n");
  ret |= ncdirect_fg(n, 0x80ff80);
  printf(" erp erp \n");
  ret |= ncdirect_styles_off(n, NCSTYLE_STANDOUT);
  printf(" erp erp \n");
  ret |= ncdirect_fg(n, 0xff8080);
  printf(" erp erp \n");
  ret |= ncdirect_cursor_right(n, geom.ws_col / 2);
  ret |= ncdirect_cursor_up(n, geom.ws_row / 2);
  printf(" erperperp! \n");
  int y = -420, x = -420;
  if(ncdirect_cursor_yx(n, &y, &x) == 0){
    fprintf(stderr, "\n\tRead cursor position: y: %d x: %d\n", y, x);
    // FIXME try a push/pop
    while(y){
      --y;
      ret |= ncdirect_cursor_up(n, 1);
      int newy;
      if(ncdirect_cursor_yx(n, &newy, NULL)){
        ret = -1;
        break;
      }
      if(newy != y){
        printf("Expected %d, got %d\n", y, newy);
        ret = -1;
        break;
      }
      fprintf(stderr, "\n\tRead cursor position: y: %d x: %d\n", newy, x);
    }
  }else{
    ret = -1;
  }
  return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
