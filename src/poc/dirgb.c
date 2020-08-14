#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <notcurses/direct.h>

static int
print_b(struct ncdirect* nc, int r, int g, int total){
  int b = total - (r + g);
  if(b > 255){
    return 0;
  }
  int ret = ncdirect_fg_rgb(nc, r, g, b);
  if(ret){
    return -1;
  }
  printf("X");
  return 0;
}

static int
print_gb(struct ncdirect* nc, int r, int total){
  for(int g = 0 ; g <= total - r && g < 256 ; g += 4){
    if(print_b(nc, r, g, total)){
      return -1;
    }
  }
  return 0;
}

static int
print_rgb(struct ncdirect* nc, int total){
  if(random() % 2){
    ncdirect_styles_off(nc, NCSTYLE_ITALIC);
  }
  if(random() % 16 == 0){
    ncdirect_styles_on(nc, NCSTYLE_ITALIC);
  }
  for(int r = 0 ; r <= total && r < 256 ; r += 4){
    if(print_gb(nc, r, total)){
      return -1;
    }
  }
  return 0;
}

// direct-mode generation of 4096 RGB foregrounds
int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  struct ncdirect* nc = ncdirect_init(NULL, stdout);
  if(!nc){
    return EXIT_FAILURE;
  }
  for(int t = 0 ; t < 768 ; t += 4){
    if(print_rgb(nc, t)){
      ncdirect_stop(nc);
      return EXIT_FAILURE;
    }
  }
  printf("\n");

  if(ncdirect_styles_set(nc, NCSTYLE_BOLD)){
    ncdirect_stop(nc);
    return EXIT_FAILURE;
  }
  for(int t = 768 ; t ; t -= 4){
    if(print_rgb(nc, t)){
      ncdirect_stop(nc);
      return EXIT_FAILURE;
    }
  }
  printf("\n");

  if(ncdirect_styles_set(nc, NCSTYLE_UNDERLINE)){
    ncdirect_stop(nc);
    return EXIT_FAILURE;
  }
  for(int t = 0 ; t < 768 ; t += 4){
    if(print_rgb(nc, t)){
      ncdirect_stop(nc);
      return EXIT_FAILURE;
    }
  }
  printf("\n");

  if(ncdirect_styles_set(nc, NCSTYLE_ITALIC)){
    ncdirect_stop(nc);
    return EXIT_FAILURE;
  }
  for(int t = 768 ; t ; t -= 4){
    if(print_rgb(nc, t)){
      ncdirect_stop(nc);
      return EXIT_FAILURE;
    }
  }
  printf("\n");

  int leny = ncdirect_dim_y(nc);
  int lenx = ncdirect_dim_x(nc);
  ncdirect_fg_default(nc);
  ncdirect_bg_default(nc);
  if(ncdirect_cursor_move_yx(nc, leny / 2, (lenx - 4) / 2)){
    ncdirect_stop(nc);
    return EXIT_FAILURE;
  }
  printf("dank\n");
  if(ncdirect_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
