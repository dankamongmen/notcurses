#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "version.h"

static struct ncselector_item items[] = {
#define SITEM(s, l) { s, l, }
  SITEM("Afrikaans", "Ek kan glas eet, dit maak my nie seer nie."),
  SITEM("AngloSax", "ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬"),
  SITEM("Japanese", "私はガラスを食べられます。それは私を傷つけません。"),
  SITEM("Kabuverdianu", "M’tá podê kumê vidru, ká stá máguame."),
  SITEM("Khmer", "ខ្ញុំអាចញុំកញ្ចក់បាន ដោយគ្មានបញ្ហារ"),
  SITEM("Lao", "ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ."),
  SITEM("Russian", "Я могу есть стекло, оно мне не вредит."),
  SITEM("Sanskrit", "kācaṃ śaknomyattum; nopahinasti mām."),
  SITEM("Braille", "⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍⠑"),
  SITEM("Tibetan", "ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད།"),
  SITEM(NULL, NULL),
#undef SITEM
};

static void
run_selector(struct notcurses* nc, struct ncselector* ns){
  static int item = 0;
  ++item;
  if(ns == NULL){
    notcurses_stop(nc);
    fprintf(stderr, "Error creating selector %d\n", item);
    exit(EXIT_FAILURE);
  }
  notcurses_render(nc);
  uint32_t keypress;
  ncinput ni;
  while((keypress = notcurses_get_blocking(nc, &ni)) != (uint32_t)-1){
    if(!ncselector_offer_input(ns, &ni)){
      if(ni.evtype == NCTYPE_RELEASE){
        continue;
      }
      switch(keypress){
        case NCKEY_ENTER: ncselector_destroy(ns, NULL); return;
        case 'M': case 'J':
          if(ncinput_ctrl_p(&ni)){
            ncselector_destroy(ns, NULL);
            return;
          }
      }
      if(keypress == 'q'){
        break;
      }
    }
    notcurses_render(nc);
  }
  ncselector_destroy(ns, NULL);
}

int main(void){
  notcurses_options opts = { };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_mice_enable(nc, NCMICE_BUTTON_EVENT);
  ncselector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 4;
  sopts.items = items;
  sopts.title = "this is truly, absolutely an awfully long example of a selector title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is no exit)";
  sopts.defidx = 1;
  sopts.boxchannels = NCCHANNELS_INITIALIZER(0x20, 0xe0, 0x40, 0x20, 0x20, 0x20);
  sopts.opchannels = NCCHANNELS_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0);
  sopts.descchannels = NCCHANNELS_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0);
  sopts.footchannels = NCCHANNELS_INITIALIZER(0xe0, 0, 0x40, 0x20, 0, 0);
  sopts.titlechannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0x80, 0, 0, 0x20);
  uint64_t bgchannels = NCCHANNELS_INITIALIZER(0, 0x20, 0, 0, 0x20, 0);
  ncchannels_set_fg_alpha(&bgchannels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&bgchannels, NCALPHA_BLEND);
  struct ncplane* n = notcurses_stdplane(nc);

  if(notcurses_canopen_images(nc)){
    struct ncvisual* ncv = ncvisual_from_file("../data/changes.jpg");
    if(!ncv){
      goto err;
    }
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_STRETCH,
      .n = n,
    };
    if(ncvisual_blit(nc, ncv, &vopts) == NULL){
      goto err;
    }
    ncvisual_destroy(ncv);
  }

  ncplane_set_fg_rgb(n, 0x40f040);
  ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, "selector widget demo");
  struct ncplane_options nopts = {
    .y = 3,
    .x = 0,
    .rows = 1,
    .cols = 1,
    .userptr = NULL,
    .name = NULL,
    .resizecb = NULL,
    .flags = 0,
  };
  struct ncplane* seln = ncplane_create(n, &nopts);
  ncplane_set_base(seln, "", 0, bgchannels);
  struct ncselector* ns = ncselector_create(seln, &sopts);
  run_selector(nc, ns);

  sopts.title = "short round title";
  seln = ncplane_create(n, &nopts);
  ncplane_set_base(seln, "", 0, bgchannels);
  ns = ncselector_create(seln, &sopts);
  run_selector(nc, ns);

  sopts.title = "short round title";
  sopts.secondary = "now this secondary is also very, very, very outlandishly long, you see";
  seln = ncplane_create(n, &nopts);
  ncplane_set_base(seln, "", 0, bgchannels);
  ns = ncselector_create(seln, &sopts);
  run_selector(nc, ns);

  sopts.title = "the whole world is watching";
  sopts.secondary = NULL;
  sopts.footer = "now this FOOTERFOOTER is also very, very, very outlandishly long, you see";
  seln = ncplane_create(n, &nopts);
  ncplane_set_base(seln, "", 0, bgchannels);
  ns = ncselector_create(seln, &sopts);
  run_selector(nc, ns);

  sopts.title = "chomps";
  sopts.secondary = NULL;
  sopts.footer = NULL;
  seln = ncplane_create(n, &nopts);
  ncplane_set_base(seln, "", 0, bgchannels);
  ns = ncselector_create(seln, &sopts);
  run_selector(nc, ns);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
