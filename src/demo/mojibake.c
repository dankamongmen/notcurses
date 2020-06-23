#include "demo.h"

static struct ncplane*
unicode52(struct ncplane* std, int y){
  struct ncplane* n = ncplane_aligned(std, 25, 72, y, NCALIGN_CENTER, NULL);
  if(ncplane_perimeter_rounded(n, 0, 0, 0) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  uint64_t channels = 0;
  channels_set_bg(&channels, 0x0);
  if(ncplane_set_base(n, " ", 0, channels) < 0 || ncplane_set_fg(n, 0xffffff)
      || ncplane_set_bg(n, 0)){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_putstr_yx(n,  1, 1, "😃😄😁😆😅😂😉😊😇😍😘😚😋😜😝😐😶😏😒😌😔😪😷😵😎😲😳😨😰😥😢😭");
  ncplane_putstr_yx(n,  2, 1, "😱😖😣😞😓😩😫😤😡😠😈👿💀💩👹👺👻👽👾😺😸😹😻😼😽🙀😿😾🙈🙉🙊💋");
  ncplane_putstr_yx(n,  3, 1, "💌💘💝💖💗💓💞💕💟💔💛💚💙💜💯💢💥💫💦💨💣💬💭💤👋✋👌👈👉👆👇👍");
  ncplane_putstr_yx(n,  4, 1, "👎✊👊👏🙌👐🙏💅💪👂👃👀👅👄👶👦👧👱👨👩👴👵🙍🙎🙅🙆💁🙋🙇👮💂👷");
  ncplane_putstr_yx(n,  5, 1, "👸👳👲👰👼🎅💆💇🚶🏃💃👯🏇🏂🏄🚣🏊🚴🚵🛀👭👫👬💏💑👪👤👥👣🐵🐒🐶");
  ncplane_putstr_yx(n,  6, 1, "🐕🐩🐺🐱🐈🐯🐅🐆🐴🐎🐮🐂🐃🐄🐷🐖🐗🐽🐏🐑🐐🐪🐫🐘🐭🐁🐀🐹🐰🐇🐻🐨");
  ncplane_putstr_yx(n,  7, 1, "🐼🐾🐔🐓🐣🐤🐥🐦🐧🐸🐊🐢🐍🐲🐉🐳🐋🐬🐟🐠🐡🐙🐚🐌🐛🐜🐝🐞💐🌸💮🌹");
  ncplane_putstr_yx(n,  8, 1, "🌺🌻🌼🌷🌱🌲🌳🌴🌵🌾🌿🍀🍁🍂🍃🍇🍈🍉🍊🍋🍌🍍🍎🍏🍐🍑🍒🍓🍅🍆🌽🍄");
  ncplane_putstr_yx(n,  9, 1, "🌰🍞🍖🍗🍔🍟🍕🍳🍲🍱🍘🍙🍚🍛🍜🍝🍠🍢🍣🍤🍥🍡🍦🍧🍨🍩🍪🎂🍰🍫🍬🍭");
  ncplane_putstr_yx(n, 10, 1, "🍮🍯🍼🍵🍶🍷🍸🍹🍺🍻🍴🔪🌍🌎🌏🌐🗾🌋🗻🏠🏡🏢🏣🏤🏥🏦🏨🏩🏪🏫🏬🏭");
  ncplane_putstr_yx(n, 11, 1, "🏯🏰💒🗼🗽🌁🌃🌄🌅🌆🌇🌉🎠🎡🎢💈🎪🚂🚃🚄🚅🚆🚇🚈🚉🚊🚝🚞🚋🚌🚍🚎");
  ncplane_putstr_yx(n, 12, 1, "🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛🚜🚲🚏🚨🚥🚦🚧🚤🚢💺🚁🚟🚠🚡🚀⏳⏰⏱⏲🕛🕧");
  ncplane_putstr_yx(n, 13, 1, "🕐🕜🕑🕝🕒🕞🕓🕟🕔🕠🕕🕡🕖🕢🕗🕣🕘🕤🕙🕥🕚🕦🌑🌒🌓🌔🌕🌖🌗🌘🌙🌚");
  ncplane_putstr_yx(n, 14, 1, "🌛🌜🌝🌞🌟🌠🌌🌀🌈🌂🔥💧🌊🎃🎄🎆🎇✨🎈🎉🎊🎋🎍🎎🎏🎐🎑🎀🎁🎫🏆🏀");
  ncplane_putstr_yx(n, 15, 1, "🏈🏉🎾🎳🎣🎽🎿🎯🎱🔮🎮🎰🎲🃏🎴🎭🎨👓👔👕👖👗👘👙👚👛👜👝🎒👞👟👠");
  ncplane_putstr_yx(n, 16, 1, "👡👢👑👒🎩🎓💄💍💎🔇🔈🔉🔊📢📣📯🔔🔕🎼🎵🎶🎤🎧📻🎷🎸🎹🎺🎻📱📲📞");
  ncplane_putstr_yx(n, 17, 1, "📟📠🔋🔌💻💽💾💿📀🎥🎬📺📷📹📼🔍🔎💡🔦🏮📔📕📖📗📘📙📚📓📒📃📜📄");
  ncplane_putstr_yx(n, 18, 1, "📰📑🔖💰💴💵💶💷💸💳💹📧📨📩📤📥📦📫📪📬📭📮📝💼📁📂📅📆📇📈📉📊");
  ncplane_putstr_yx(n, 19, 1, "📋📌📍📎📏📐🔒🔓🔏🔐🔑🔨🔫🔧🔩🔗🔬🔭📡💉💊🚪🚽🚿🛁🚬🗿🏧🚮🚰🚹🚺");
  ncplane_putstr_yx(n, 20, 1, "🚻🚼🚾🛂🛃🛄🛅🚸🚫🚳🚭🚯🚱🚷📵🔞🔃🔄🔙🔚🔛🔜🔝🔯⛎🔀🔁🔂⏩⏭⏯⏪⏮");
  ncplane_putstr_yx(n, 21, 1, "🔼⏫🔽⏬🎦🔅🔆📶📳📴➕➖➗❓❔❕💱💲🔱📛🔰✅❌❎➰➿🔟🔠🔡🔢🔣🔤");
  ncplane_putstr_yx(n, 22, 1, "🅰🆎🅱🆑🆒🆓🆔🆕🆖🅾🆗🆘🆙🆚🈁🈂🈷🈶🉐🈹🈲🉑🈸🈴🈳🈺🈵🔴🔵🔶🔷🔸🔹🔺");
  ncplane_putstr_yx(n, 23, 1, "🔻💠🔘🔳🔲🏁🚩🎌⛧⛤⛢⛦⛥");
  const char SUMMARY[] = "[Unicode 5.2 (2009), 722 codepoints]";
  const int x = ncplane_align(n, NCALIGN_RIGHT, strlen(SUMMARY) + 2);
  ncplane_putstr_yx(n, 24, x, SUMMARY);
  return n;
}

struct ncplane*
maketitle(struct ncplane* std, int dimx){
  struct ncplane* title = ncplane_aligned(std, 3, dimx, 1, NCALIGN_CENTER, NULL);
  if(title == NULL){
    return NULL;
  }
  uint64_t channels = 0;
  channels_set_bg(&channels, 0x0);
  if(ncplane_set_base(title, " ", 0, channels) < 0 || ncplane_set_fg(title, 0xffffff)
      || ncplane_set_bg(title, 0)){
    ncplane_destroy(title);
    return NULL;
  }
  if(ncplane_putstr_aligned(title, 0, NCALIGN_CENTER, "mojibake 文字化けmodʑibake") < 0){
    ncplane_destroy(title);
    return NULL;
  }
  if(ncplane_putstr_aligned(title, 1, NCALIGN_CENTER, "the display of emoji depends upon terminal, font, and font rendering engine.") < 0){
    ncplane_destroy(title);
    return NULL;
  }
  if(ncplane_putstr_aligned(title, 2, NCALIGN_CENTER, "not all symbols are emoji, and not all emoji map to a single code point.") < 0){
    ncplane_destroy(title);
    return NULL;
  }
  return title;
}

int mojibake_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane* title = maketitle(std, dimx);
  if(title == NULL){
    return -1;
  }
  struct ncplane* planes[] = {
    unicode52(std, dimy - 1),
  };
  // scroll the various planes up from the bottom. none are onscreen save the
  // first, which starts at the bottom. each time one clears, we bring the
  // next one onscreen; at each step, we move all onscreen up by one row. when
  // the last one exits via the top, we're done.
  unsigned topmost = 0; // index of the topmost visible panel
  struct timespec stepdelay;
  // two seconds onscreen per plane at standard (1s) delay
  timespec_div(&demodelay, dimy / 2, &stepdelay);
  ncplane_move_below(planes[0], title);
  do{
    unsigned u = topmost;
    do{
      int y, x, leny;
      ncplane_yx(planes[u], &y, &x);
      if(ncplane_move_yx(planes[u], y - 1, x)){
        goto err;
      }
      ncplane_dim_yx(planes[u], &leny, NULL);
      if(leny + y + 1 == 0){
        ++topmost;
      }
      if(leny + y + 1 == dimy - 1){
        // FIXME bring next one on
      }
      ++u;
    }while(u < sizeof(planes) / sizeof(*planes));
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &stepdelay);
  }while(topmost < sizeof(planes) / sizeof(*planes));
  for(unsigned u = 0 ; u < sizeof(planes) / sizeof(*planes) ; ++u){
    ncplane_destroy(planes[u]);
  }
  ncplane_destroy(title);
  return 0;

err:
  for(unsigned u = 0 ; u < sizeof(planes) / sizeof(*planes) ; ++u){
    ncplane_destroy(planes[u]);
  }
  ncplane_destroy(title);
  return -1;
}
