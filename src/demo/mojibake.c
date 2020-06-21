#include "demo.h"

static struct ncplane*
unicode60(struct ncplane* std, int y, int dimx){
  struct ncplane* n = ncplane_aligned(std, 25, dimx - 8, y, NCALIGN_CENTER, NULL);
  if(ncplane_perimeter_rounded(n, 0, 0, 0) < 0){
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
  return n;
}

int mojibake_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_set_fg(std, 0xffffff);
  if(ncplane_putstr_aligned(std, 2, NCALIGN_CENTER, "mojibake 文字化けmodʑibake") < 0){
    return -1;
  }
  struct ncplane* u60 = unicode60(std, 4, dimx);
  DEMO_RENDER(nc);
  demo_nanosleep(nc, &demodelay);
  return 0;
}
