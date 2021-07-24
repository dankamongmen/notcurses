#include <limits.h>
#include <notcurses/direct.h>

int main(void){
  srand(time(NULL)); // gross
  long guess, secret = rand();
  struct ncdirect* n = ncdirect_core_init(NULL, stdout, NCDIRECT_OPTION_INHIBIT_CBREAK);
  if(n == NULL){
    return EXIT_FAILURE;
  }
  int r = 0;
  do{
    if(!(r |= (ncdirect_set_fg_default(n)))){
      if(!(r |= (printf("Guess the long: ") < 0))){
        if(!(r |= fflush(stdout))){
          int rargs = scanf("%ld", &guess); // super shitty to the max
          if(rargs != 1){
            r = -1;
            break;
          }
          int offoom = labs(__builtin_clzl(guess) - __builtin_clzl(secret));
          if(guess > secret){
            r |= ncdirect_set_fg_rgb8(n, 0x40, 0x80, offoom * 6);
            r |= (printf("\tLOL jabronies guess %ld. Too high!\n", guess) < 0);
          }else if(guess < secret){
            r |= ncdirect_set_fg_rgb8(n, offoom * 6, 0x80, 0x40);
            r |= (printf("\tSpineless worm! %ld? Too low!\n", guess) < 0);
          }
        }
      }
    }
  }while(!r && guess != secret);
  if(r || printf("You enjoy 20/20 vision into the minds of antimen!\n") < 0){
    ncdirect_stop(n);
    return EXIT_FAILURE;
  }
  return ncdirect_stop(n) ? EXIT_FAILURE : EXIT_SUCCESS;
}
