#include <stdlib.h>
#include <locale.h>
#include <notcurses/direct.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  struct ncdirect* n = ncdirect_init(NULL, stdout, 0);
  putchar('\n');
  for(int i = 0 ; i < 15 ; ++i){
    uint64_t c1 = 0, c2 = 0;
    channels_set_fg_rgb8(&c1, 0x0, 0x10 * i, 0xff);
    channels_set_fg_rgb8(&c2, 0x10 * i, 0x0, 0x0);
    if(ncdirect_hline_interp(n, "-", i, c1, c2) < i){
      return EXIT_FAILURE;
    }
    ncdirect_fg_default(n);
    ncdirect_bg_default(n);
    putchar('\n');
  }
  for(int i = 0 ; i < 15 ; ++i){
    uint64_t c1 = 0, c2 = 0;
    channels_set_fg_rgb8(&c1, 0x0, 0x10 * i, 0xff);
    channels_set_fg_rgb8(&c2, 0x10 * i, 0x0, 0x0);
    if(ncdirect_vline_interp(n, "|", i, c1, c2) < i){
      return EXIT_FAILURE;
    }
    ncdirect_fg_default(n);
    ncdirect_bg_default(n);
    if(i < 14){
      if(ncdirect_cursor_up(n, i)){
        return EXIT_FAILURE;
      }
    }
  }
  printf("\n");
  uint64_t ul, ur, ll, lr;
  ul = ur = ll = lr = 0;
  channels_set_fg_rgb8(&ul, 0xff, 0x0, 0xff);
  channels_set_fg_rgb8(&ur, 0x0, 0xff, 0x0);
  channels_set_fg_rgb8(&ll, 0x0, 0x0, 0xff);
  channels_set_fg_rgb8(&lr, 0xff, 0x0, 0x0);
  if(ncdirect_rounded_box(n, ul, ur, ll, lr, 10, 10, 0) < 0){
    return EXIT_FAILURE;
  }
  ncdirect_cursor_up(n, 9);
  if(ncdirect_double_box(n, ul, ur, ll, lr, 10, 20, 0) < 0){
    return EXIT_FAILURE;
  }
  if(ncdirect_stop(n)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
