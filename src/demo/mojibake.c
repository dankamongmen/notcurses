#include "demo.h"

static struct ncplane*
mojiplane(struct ncplane* title, int y, int rows, const char* summary){
  struct ncplane* n = ncplane_aligned(title, rows, 64, y, NCALIGN_CENTER, NULL);
  if(ncplane_perimeter_rounded(n, 0, 0, 0) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  uint64_t channels = 0;
  channels_set_bg(&channels, 0x0);
  if(ncplane_set_fg(n, 0x40d0d0) || ncplane_set_bg(n, 0)){
    ncplane_destroy(n);
    return NULL;
  }
  const int x = ncplane_align(n, NCALIGN_RIGHT, strlen(summary) + 2);
  if(ncplane_putstr_yx(n, rows - 1, x, summary) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_set_base(n, " ", 0, channels) < 0 || ncplane_set_fg(n, 0x40d040)){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_move_below(n, title);
  return n;
}

static struct ncplane*
unicode52(struct ncplane* title, int y){
  const char SUMMARY[] = "[Unicode 5.2 (2009), Emoji 1.0 (2015), 24 codepoints]";
  const int ROWS = 5;
  struct ncplane* n = mojiplane(title, y, ROWS, SUMMARY);
  if(n == NULL){
    return NULL;
  }
  ncplane_putstr_yx(n, 1, 1, "\u26ea (\u26ea\ufe0f) \u26f2 (\u26f2\ufe0f)"
                             "\u26fa (\u26fa\ufe0f) \u2668 (\u2668\ufe0f)"
                             "\u26fd (\u26fd\ufe0f) \u2693 (\u2693\ufe0f)"
                             "\u26f5 (\u26f5\ufe0f) \u2600 (\u2600\ufe0f)");
  ncplane_putstr_yx(n, 2, 1, "\u26c5 (\u26c5\ufe0f) \u2614 (\u2614\ufe0f)"
                             "\u26a1 (\u26a1\ufe0f) \u26c4 (\u26c4\ufe0f)"
                             "\u26be (\u26b3\ufe0f) \u26d4 (\u26d4\ufe0f)"
                             "\u2b06 (\u2b06\ufe0f) \u2b07 (\u2b07\ufe0f)");
  ncplane_putstr_yx(n, 3, 1, "\u2b05 (\u2b05\ufe0f) \u26ce (\u26ce\ufe0f)"
                             "\u203c (\u203c\ufe0f) \u2049 (\u2049\ufe0f)"
                             "\xf0\x9f\x85\xbf (\xf0\x9f\x85\xbf\ufe0f)"
                             "\xf0\x9f\x88\xaf (\xf0\x9f\x88\xaf\ufe0f)"
                             "\xf0\x9f\x88\x9a (\xf0\x9f\x88\x9a\ufe0f)"
                             "\u3299 (\u3299\ufe0f)");
  return n;
}

static struct ncplane*
unicode60(struct ncplane* title, int y){
  const char SUMMARY[] = "[Unicode 6.0 (2010), 722 codepoints]";
  const int ROWS = 25;
  struct ncplane* n = mojiplane(title, y, ROWS, SUMMARY);
  if(n == NULL){
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
  ncplane_putstr_yx(n, 20, 1, "🚻🚼🚾🛂🛃🛄🛅🚸🚫🚳🚭🚯🚱🚷📵🔞🔃🔄🔙🔚🔛🔜🔝🔯⛎🔀🔁🔂⏩🔼⏪");
  ncplane_putstr_yx(n, 21, 1, "⏭\ufe0f⏮\ufe0f⏫🔽⏬🎦🔅🔆📶📳📴➕➖➗❓❔❕💱💲🔱📛🔰✅❌❎➰➿🔟🔠🔡🔢");
  ncplane_putstr_yx(n, 22, 1, "⏯\ufe0f🔣🔤🅰🆎🅱🆑🆒🆓🆔🆕🆖🅾🆗🆘🆙🆚🈁🈂🈷🈶🉐🈹🈲🉑🈸🈴🈳🈺🈵🔴🔵🔶🔷");
  ncplane_putstr_yx(n, 23, 1, "🔸🔹🔺🔻💠🔘🔳🔲🏁🚩🎌⛧⛤⛢⛦⛥");
  return n;
}

static struct ncplane*
unicode1emoji1(struct ncplane* title, int y){
  const char SUMMARY[] = "[Unicode 1.1 (1993) / Emoji 1.0 (2015), 16 codepoints]";
  const int ROWS = 4;
  struct ncplane* n = mojiplane(title, y, ROWS, SUMMARY);
  if(n == NULL){
    return NULL;
  }
  ncplane_putstr_yx(n, 1, 1, "\u2764 (\u2764\ufe0f)"
                             "\u2709 (\u2709\ufe0f)"
                             "\u270f (\u270f\ufe0f)"
                             "\u2712 (\u2712\ufe0f)"
                             "\u2195 (\u2195\ufe0f) \u2194 (\u2194\ufe0f)"
                             "\u2716 (\u2716\ufe0f) \u2733 (\u2733\ufe0f)"
                             "\u2734 (\u2734\ufe0f) \u2747 (\u2747\ufe0f)");
  ncplane_putstr_yx(n, 2, 1, "\u2660 (\u2660\ufe0f) \u2665 (\u2665\ufe0f)"
                             "\u2666 (\u2666\ufe0f) \u2663 (\u2663\ufe0f)"
                             "\u260e (\u260e\ufe0f) \u27a1 (\u27a1\ufe0f)");
  return n;
}

static struct ncplane*
unicode7emoji2(struct ncplane* title, int y){
  const char SUMMARY[] = "[Unicode 7.0 (2014) / Emoji 2.0 (2015), 1 codepoint]";
  const int ROWS = 3;
  struct ncplane* n = mojiplane(title, y, ROWS, SUMMARY);
  if(n == NULL){
    return NULL;
  }
  ncplane_putstr_yx(n, 1, 1, "\xf0\x9f\x97\xa8 (\xf0\x9f\x97\xa8\ufe0f)");
  return n;
}

static struct ncplane*
unicode13(struct ncplane* title, int y){
  const char SUMMARY[] = "[Unicode 13.0 (2020), 56 codepoints]";
  const int ROWS = 4;
  struct ncplane* n = mojiplane(title, y, ROWS, SUMMARY);
  if(n == NULL){
    return NULL;
  }
  ncplane_putstr_yx(n, 1, 1, "\xf0\x9f\xa5\xb2\xf0\x9f\xa5\xb8\xf0\x9f\xa4\x8c"
                             "\xf0\x9f\xab\x80\xf0\x9f\xab\x81\xf0\x9f\xa5\xb7"
                             "\xf0\x9f\xab\x82\xf0\x9f\xa6\xac\xf0\x9f\xa6\xa3"
                             "\xf0\x9f\xa6\xab\xf0\x9f\xa6\xa4\xf0\x9f\xaa\xb6"
                             "\xf0\x9f\xa6\xad\xf0\x9f\xaa\xb2\xf0\x9f\xaa\xb3"
                             "\xf0\x9f\xaa\xb0\xf0\x9f\xaa\xb1\xf0\x9f\xaa\xb4"
                             "\xf0\x9f\xab\x90\xf0\x9f\xab\x92\xf0\x9f\xab\x91"
                             "\xf0\x9f\xab\x93\xf0\x9f\xab\x94\xf0\x9f\xab\x95"
                             "\xf0\x9f\xab\x96\xf0\x9f\xa7\x8b\xf0\x9f\xaa\xa8"
                             "\xf0\x9f\xaa\xb5\xf0\x9f\x9b\x96\xf0\x9f\x9b\xbb"
                             "\xf0\x9f\x9b\xbc\xf0\x9f\xaa\x84");
  ncplane_putstr_yx(n, 2, 1, "\xf0\x9f\xaa\x85\xf0\x9f\xaa\x86\xf0\x9f\xaa\xa1"
                             "\xf0\x9f\xaa\xa2\xf0\x9f\xa9\xb4\xf0\x9f\xaa\x96"
                             "\xf0\x9f\xaa\x97\xf0\x9f\xaa\x98\xf0\x9f\xaa\x99"
                             "\xf0\x9f\xaa\x83\xf0\x9f\xaa\x9a\xf0\x9f\xaa\x9b"
                             "\xf0\x9f\xaa\x9d\xf0\x9f\xaa\x9c\xf0\x9f\x9b\x97"
                             "\xf0\x9f\xaa\x9e\xf0\x9f\xaa\x9f\xf0\x9f\xaa\xa0"
                             "\xf0\x9f\xaa\xa4\xf0\x9f\xaa\xa3\xf0\x9f\xaa\xa5"
                             "\xf0\x9f\xaa\xa6\xf0\x9f\xaa\xa7\xe2\x9a\xa7");
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
  if(ncplane_putstr_aligned(title, 0, NCALIGN_CENTER, "mojibake 文字化けmodʑibake (english: \"garbled\")") < 0){
    ncplane_destroy(title);
    return NULL;
  }
  if(ncplane_putstr_aligned(title, 1, NCALIGN_CENTER, "Display of emoji depends upon terminal, font, and font rendering engine.") < 0){
    ncplane_destroy(title);
    return NULL;
  }
  if(ncplane_putstr_aligned(title, 2, NCALIGN_CENTER, "Not all symbols are emoji, and not all emoji map to a single code point.") < 0){
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
    unicode1emoji1(title, dimy - 1),
    unicode52(title, dimy + 1),
    unicode60(title, dimy + 1),
    unicode7emoji2(title, dimy + 1),
    unicode13(title, dimy + 1),
  };
  // scroll the various planes up from the bottom. none are onscreen save the
  // first, which starts at the bottom. each time one clears, we bring the
  // next one onscreen; at each step, we move all onscreen up by one row. when
  // the last one exits via the top, we're done.
  unsigned topmost = 0; // index of the topmost visible panel
  struct timespec stepdelay;
  // two seconds onscreen per plane at standard (1s) delay
  timespec_div(&demodelay, dimy / 2, &stepdelay);
  do{
    unsigned u = topmost;
    do{
      int y, x, leny;
      ncplane_yx(planes[u], &y, &x);
      if(y >= dimy){
        break;
      }
      if(ncplane_move_yx(planes[u], y - 1, x)){
        goto err;
      }
      ncplane_dim_yx(planes[u], &leny, NULL);
      if(leny + y + 1 == 0){
        ++topmost;
      }
      if(leny + y + 1 == dimy - 1){
        if(u + 1 < sizeof(planes) / sizeof(*planes)){
          if(ncplane_move_yx(planes[u + 1], dimy - 1, x)){
            goto err;
          }
        }
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
