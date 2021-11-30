#include <assert.h>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unictype.h>
#include "demo.h"

// Fill up the screen with as much crazy Unicode as we can, and then set a
// gremlin loose, looking to brighten up the world.

static struct ncplane*
mathplane(struct notcurses* nc){
  struct ncplane* stdn = notcurses_stdplane(nc);
  unsigned dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  const int HEIGHT = 9;
  const int WIDTH = dimx;
  struct ncplane_options nopts = {
    .y = dimy - HEIGHT - 1,
    .x = dimx - WIDTH,
    .rows = HEIGHT,
    .cols = WIDTH,
  };
  struct ncplane* n = ncplane_create(stdn, &nopts);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb(&channels, 0x2b50c8); // metallic gold, inverted
  ncchannels_set_fg_alpha(&channels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, "", 0, channels);
  ncplane_set_fg_rgb(n, 0xd4af37); // metallic gold
  ncplane_set_bg_rgb(n, 0x0);
  if(n){
    ncplane_printf_aligned(n, 0, NCALIGN_RIGHT, "∮E⋅da=Q,n→∞,∑f(i)=∏g(i)⎧⎡⎛       ⎞⎤⎫");
    ncplane_printf_aligned(n, 1, NCALIGN_RIGHT, "⎪⎢⎜ 8πG   ⎟⎥⎪");
    ncplane_printf_aligned(n, 2, NCALIGN_RIGHT, "∀x∈ℝ:⌈x⌉=−⌊−x⌋,α∧¬β=¬(¬α∨β)⎪⎢⎜ ───Tμν⎟⎥⎪");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "⎪⎢⎜  c⁴   ⎟⎥⎪");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, "ℕ⊆ℕ₀⊂ℤ⊂ℚ⊂ℝ⊂ℂ(z̄=ℜ(z)−ℑ(z)⋅𝑖)⎨⎢⎜       ⎟⎥⎬");
    ncplane_printf_aligned(n, 5, NCALIGN_RIGHT, "⎪⎢⎜ ∞     ⎟⎥⎪");
    ncplane_printf_aligned(n, 6, NCALIGN_RIGHT, "⊥<a≠b≡c≤d≪⊤⇒(⟦A⟧⇔⟪B⟫)⎪⎢⎜ ⎲     ⎟⎥⎪");
    ncplane_printf_aligned(n, 7, NCALIGN_RIGHT, "⎪⎢⎜ ⎳aⁱ-bⁱ⎟⎥⎪");
    ncplane_printf_aligned(n, 8, NCALIGN_RIGHT, "2H₂+O₂⇌2H₂O,R=4.7kΩ,⌀200µm⎩⎣⎝i=1    ⎠⎦⎭");
  }
  return n;
}

// the closer the coordinate is (lower distance), the more we lighten the cell
static inline int
lighten(struct ncplane* n, nccell* c, int distance, int y, int x){
  if(nccell_wide_right_p(c)){ // not really a character
    return 0;
  }
  unsigned r, g, b;
  nccell_fg_rgb8(c, &r, &g, &b);
  r += rand() % (64 / (2 * distance + 1) + 1);
  g += rand() % (64 / (2 * distance + 1) + 1);
  b += rand() % (64 / (2 * distance + 1) + 1);
  nccell_set_fg_rgb8_clipped(c, r, g, b);
  return ncplane_putc_yx(n, y, x, c);
}

static inline int
lightup_surrounding_cells(struct ncplane* n, nccell* lightup, int y, int x){
  lighten(n, lightup, 0, y, x);
  nccell_release(n, lightup);
  return 0;
}

typedef struct worm {
  nccell lightup;
  int x, y;
  int prevx, prevy;
} worm;

static void
init_worm(worm* s, int dimy, int dimx){
  nccell_init(&s->lightup);
  s->y = rand() % dimy;
  s->x = rand() % dimx;
  s->prevx = 0;
  s->prevy = 0;
}

static int
wormy_top(struct notcurses* nc, worm* s){
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_at_yx_cell(n, s->y, s->x, &s->lightup);
  if(lightup_surrounding_cells(n, &s->lightup, s->y, s->x)){
    return -1;
  }
  return 0;
}

static int
wormy(worm* s, int dimy, int dimx){
  int oldy, oldx;
  oldy = s->y;
  oldx = s->x;
  do{ // force a move
    s->y = oldy;
    s->x = oldx;
    int direction = rand() % 4;
    switch(direction){
      case 0: --s->y; break;
      case 1: ++s->x; break;
      case 2: ++s->y; break;
      case 3: --s->x; break;
    }
    if(s->y <= 1){
      s->y = dimy - 1;
    }
    if(s->y >= dimy){
      s->y = 0;
    }
    if(s->x <= 0){
      s->x = dimx - 1;
    }
    if(s->x >= dimx){
      s->x = 0;
    }
  }while((oldx == s->x && oldy == s->y) || (s->x == s->prevx && s->y == s->prevy));
  s->prevy = oldy;
  s->prevx = oldx;
  return 0;
}

struct worm_ctx {
  int wormcount;
  worm* worms;
};

int init_worms(struct worm_ctx* wctx, int dimy, int dimx){
  if((wctx->wormcount = (dimy * dimx) / 800) == 0){
    wctx->wormcount = 1;
  }
  if((wctx->worms = malloc(sizeof(*wctx->worms) * wctx->wormcount)) == NULL){
    return -1;
  }
  for(int s = 0 ; s < wctx->wormcount ; ++s){
    init_worm(&wctx->worms[s], dimy, dimx);
  }
  return 0;
}

// the whiteworms wander around aimlessly, lighting up cells around themselves
static int
worm_move(struct notcurses* nc, struct worm_ctx* wctx, int dimy, int dimx){
  for(int s = 0 ; s < wctx->wormcount ; ++s){
    if(wormy_top(nc, &wctx->worms[s])){
      return -1;
    }
  }
  int err;
  if( (err = demo_render(nc)) ){
    return err;
  }
  for(int s = 0 ; s < wctx->wormcount ; ++s){
    if(wormy(&wctx->worms[s], dimy, dimx)){
      return -1;
    }
  }
  return 0;
}

static int
message(struct ncplane* n, int maxy, int maxx, int num, int total,
        int bytes_out, int egs_out, int cols_out){
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, "", 0, channels);
  ncplane_set_fg_rgb8(n, 255, 255, 255);
  ncplane_set_bg_rgb8(n, 32, 64, 32);
  channels = 0;
  ncchannels_set_fg_rgb8(&channels, 255, 255, 255);
  ncchannels_set_bg_rgb8(&channels, 32, 64, 32);
  ncplane_cursor_move_yx(n, 2, 0);
  if(ncplane_rounded_box(n, 0, channels, 4, 56, 0)){
    return -1;
  }
  // bottom handle
  ncplane_putegc_yx(n, 4, 17, "┬", NULL);
  ncplane_putegc_yx(n, 5, 17, "│", NULL);
  ncplane_putegc_yx(n, 6, 17, "╰", NULL);
  nccell hl = NCCELL_TRIVIAL_INITIALIZER;
  nccell_load(n, &hl, "─");
  nccell_set_fg_rgb8(&hl, 255, 255, 255);
  nccell_set_bg_rgb8(&hl, 32, 64, 32);
  ncplane_hline(n, &hl, 57 - 18 - 1);
  ncplane_putegc_yx(n, 6, 56, "╯", NULL);
  ncplane_putegc_yx(n, 5, 56, "│", NULL);
  ncplane_putegc_yx(n, 4, 56, "┤", NULL);

  // top handle
  ncplane_putegc_yx(n, 2, 3, "╨", NULL);
  ncplane_putegc_yx(n, 1, 3, "║", NULL);
  ncplane_putegc_yx(n, 0, 3, "╔", NULL);
  nccell_load(n, &hl, "═");
  ncplane_hline(n, &hl, 20 - 4 - 1);
  nccell_release(n, &hl);
  ncplane_putegc_yx(n, 0, 19, "╗", NULL);
  ncplane_putegc_yx(n, 1, 19, "║", NULL);
  ncplane_putegc_yx(n, 2, 19, "╨", NULL);
  ncplane_set_fg_rgb8(n, 64, 128, 240);
  ncplane_set_bg_rgb8(n, 32, 64, 32);
  ncplane_on_styles(n, NCSTYLE_ITALIC);
  ncplane_printf_yx(n, 5, 18, " bytes: %05d EGCs: %05d cols: %05d ", bytes_out, egs_out, cols_out);
  ncplane_printf_yx(n, 1, 4, " %03dx%03d (%d/%d) ", maxx, maxy, num + 1, total);
  ncplane_off_styles(n, NCSTYLE_ITALIC);
  ncplane_set_fg_rgb8(n, 224, 128, 224);
  ncplane_putstr_yx(n, 3, 1, " 🎆🔥 unicode 14, resize awareness, 24b truecolor…🔥🎆 ");
  ncplane_set_fg_rgb8(n, 255, 255, 255);
  return 0;
}

// print the EGCs of the UTF8 string on the standard plane, wrapping around
// the borders. the plane is not set to scroll, so we handle it ourselves.
static int
dostring(struct ncplane* n, const char** s, uint32_t rgb,
         unsigned maxy, unsigned maxx,
         unsigned* egcs, unsigned* cols, unsigned* bytes){
  size_t idx = 0;
  unsigned y, x;
  ncplane_cursor_yx(n, &y, &x);
  while((*s)[idx]){ // each multibyte char of string
    if(ncplane_set_fg_rgb8(n, ncchannel_r(rgb), ncchannel_g(rgb), ncchannel_b(rgb))){
      return -1;
    }
    if(x >= maxx){
      x = 0;
      ++y;
    }
    if(y >= maxy){
      break;
    }
    wchar_t wcs;
    mbstate_t mbstate;
    memset(&mbstate, 0, sizeof(mbstate));
    int eaten = mbrtowc(&wcs, &(*s)[idx], MB_CUR_MAX + 1, &mbstate);
    if(eaten < 0){
      return -1;
    }
    if(iswspace(wcs)){
      idx += eaten;
      continue;
    }
    size_t ulen = 0;
    int r = 0;
    if((r = ncplane_putegc(n, &(*s)[idx], &ulen)) <= 0){
      return 0;
    }
    ncplane_cursor_yx(n, &y, &x);
    idx += ulen;
    *bytes += ulen;
    *cols += r;
    ++*egcs;
  }
  return 0;
}

// Much of this text comes from http://kermitproject.org/utf8.html
int whiteout_demo(struct notcurses* nc, uint64_t startns){
  static const char* strs[] = {
    "Война и мир",
    "Бра́тья Карама́зовы",
    "Час сэканд-хэнд",
    "Tonio Kröger",
    "Meg tudom enni az üveget, nem lesztőle bajom",
    "Voin syödä lasia, se ei vahingoita minua",
    "Sáhtán borrat lása, dat ii leat bávččas",
    "Мон ярсан суликадо, ды зыян эйстэнзэ а ули",
    "Mie voin syvvä lasie ta minla ei ole kipie",
    "Minä voin syvvä st'oklua dai minule ei ole kibie",
    "Ma võin klaasi süüa, see ei tee mulle midagi",
    "Es varu ēst stiklu, tas man nekaitē",
    "Aš galiu valgyti stiklą ir jis manęs nežeidži",
    "Mohu jíst sklo, neublíží mi",
    "Môžem jesť sklo. Nezraní ma",
    "Mogę jeść szkło i mi nie szkodzi",
    "Lahko jem steklo, ne da bi mi škodovalo",
    "Ja mogu jesti staklo, i to mi ne šteti",
    "Ја могу јести стакло, и то ми не штети",
    "Можам да јадам стакло, а не ме штета",
    "Я могу есть стекло, оно мне не вредит",
    "Я магу есці шкло, яно мне не шкодзіць",
    "Osudy dobrého vojáka Švejka za světové války",
    "kācaṃ śaknomyattum; nopahinasti mām",
    "ὕαλον ϕαγεῖν δύναμαι· τοῦτο οὔ με βλάπτει",
    "Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα",
    "Vitrum edere possum; mihi non nocet",
    "iℏ∂∂tΨ=−ℏ²2m∇2Ψ+VΨ",
    "Je puis mangier del voirre. Ne me nuit",
    "Je peux manger du verre, ça ne me fait pas mal",
    "Pòdi manjar de veire, me nafrariá pas",
    "J'peux manger d'la vitre, ça m'fa pas mal",
    "Dji pou magnî do vêre, çoula m' freut nén må",
    "Ch'peux mingi du verre, cha m'foé mie n'ma",
    "F·ds=ΔE",
    "Mwen kap manje vè, li pa blese'm",
    "Kristala jan dezaket, ez dit minik ematen",
    "Puc menjar vidre, que no em fa mal",
    "overall there is a smell of fried onions",
    "Puedo comer vidrio, no me hace daño",
    "Puedo minchar beire, no me'n fa mal",
    "Eu podo xantar cristais e non cortarme",
    "Posso comer vidro, não me faz mal",
    "Posso comer vidro, não me machuca",
    "ஸீரோ டிகிரி",
    "بين القصرين",
    "قصر الشوق",
    "السكرية",
    "三体",
    "血的神话公元年湖南道县文革大屠杀纪实",
    "三国演义",
    "紅樓夢",
    "Hónglóumèng",
    "红楼梦",
    "महाभारतम्",
    "Mahābhāratam",
    " रामायणम्",
    "Rāmāyaṇam",
    "القرآن",
    "תּוֹרָה",
    "תָּנָ״ךְ",
    "Σίβνλλα τί ϴέλεις; respondebat illa: άπο ϴανεΐν ϴέλω",
    "① На всей земле был один язык и одно наречие.",
    "② А кад отидоше од истока, нађоше равницу у земљи сенарској, и населише се онде.",
    "③ І сказалі адно аднаму: наробім цэглы і абпалім агнём. І стала ў іх цэгла замест камянёў, а земляная смала замест вапны.",
    "④ І сказали вони: Тож місто збудуймо собі, та башту, а вершина її аж до неба. І вчинімо для себе ймення, щоб ми не розпорошилися по поверхні всієї землі.",
    "A boy has never wept nor dashed a thousand kim",
    "⑤ Господ слезе да ги види градот и кулата, што луѓето ги градеа.",
    "⑥ И҆ речѐ гдⷭ҇ь: сѐ, ро́дъ є҆ди́нъ, и҆ ѹ҆стнѣ̀ є҆ди҄нѣ всѣ́хъ, и҆ сїѐ нача́ша твори́ти: и҆ нн҃ѣ не ѡ҆скꙋдѣ́ютъ ѿ ни́хъ всѧ҄, є҆ли҄ка а́҆ще восхотѧ́тъ твори́ти.",
    "⑦ Ⱂⱃⰻⰻⰴⱑⱅⰵ ⰺ ⰺⰸⱎⰵⰴⱎⰵ ⱄⰿⱑⱄⰻⰿⱏ ⰺⰿⱏ ⱅⱆ ⱔⰸⱏⰹⰽⰻ ⰺⱈⱏ · ⰴⰰ ⱀⰵ ⱆⱄⰾⱏⰹⱎⰰⱅⱏ ⰽⱁⰶⰴⱁ ⰴⱃⱆⰳⰰ ⱄⰲⱁⰵⰳⱁ ⁖⸏",
    "काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम्",
    "色は匂へど 散りぬるを 我が世誰ぞ 常ならむ 有為の奥山 今日越えて 浅き夢見じ 酔ひもせず",
    "いろはにほへど　ちりぬるを わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　ゑひもせず",
    "मलाई थाहा छैन । म यहाँ काम मात्र गर्छु ",
    "ብርሃነ ዘርኣይ",
    "ኃይሌ ገብረሥላሴ",
    "ᓱᒻᒪᓂᒃᑯᐊ ᐃᓄᑦᑎᑐᑐᐃᓐᓇᔭᙱᓚᑦ",
    "ði ıntəˈnæʃənəl fəˈnɛtık əsoʊsiˈeıʃn",
    "((V⍳V)=⍳⍴V)/V←,V    ⌷←⍳→⍴∆∇⊃‾⍎⍕⌈",
    "Eڿᛯℇ✈ಅΐʐ𝍇Щঅ℻ ⌬⌨ ⌣₰ ⠝ ‱ ‽ ח ֆ ∜ ⨀ ĲႪ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣⧒ ₔ ⅷ ﭗ ゛〃・ ↂ ﻩ ✞ ℼ ⌧",
    "M' podê cumê vidru, ca ta maguâ-m'",
    "Ami por kome glas anto e no ta hasimi daño",
    "六四事件八九民运动态网自由门天安门天安门法轮功李洪志六四天安门事件天安门大屠杀反右派斗争大跃进政策文化大革命人权民运自由独立I多党制台湾台湾T中华民国西藏土伯特唐古特达赖喇嘛法轮功新疆维吾尔自治区诺贝尔和平奖刘暁波民主言论思想反共反革命抗议运动骚乱暴乱骚扰扰乱抗暴平反维权示威游行李洪志法轮大法大法弟子强制断种强制堕胎民族净化人体实验肃清胡耀邦赵紫阳魏京生王丹还政于民和平演变激流中国北京之春大纪元时报评论共产党独裁专制压制统监视镇压迫害 侵略掠夺破坏拷问屠杀活摘器官诱拐买卖人口游进走私毒品卖淫春画赌博六合彩天安门天安门法轮功李洪志刘晓波动态网自由门",
    "Posso mangiare il vetro e non mi fa male",
    "زَّ وَجَلَّ فَمَا وَجَدْنَا فِيهِ مِنْ حَلاَلٍ اسْتَحْلَلْنَاهُ وَمَا وَجَدْنَا فِيهِ مِنْ حَرَامٍ حَرَّمْنَاهُ . أَلاَ وَإِنَّ مَا حَرَّمَ رَسُولُ اللَّهِ ـ صلى الله عليه وسلم ـ مِثْلُ مَا حَرَّمَ اللَّ",
    "śrī-bhagavān uvāca kālo 'smi loka-kṣaya-kṛt pravṛddho lokān samāhartum iha pravṛttaḥ ṛte 'pi tvāṁ na bhaviṣyanti sarve ye 'vasthitāḥ pratyanīkeṣu yodhāḥ",
    "الحرام لذاتهالحرام لغيره",
    "Je suis Charli",
    "Sôn bôn de magnà el véder, el me fa minga mal",
    "Ewige Blumenkraft",
    "HEUTE DIE WELT MORGENS DAS SONNENSYSTEM",
    "Me posso magna' er vetro, e nun me fa male",
    "M' pozz magna' o'vetr, e nun m' fa mal",
    "μῆλον τῆς Ἔριδος",
    "verwirrung zweitracht unordnung beamtenherrschaft grummet",
    "Mi posso magnare el vetro, no'l me fa mae",
    "Pòsso mangiâ o veddro e o no me fà mâ",
    "Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn",
    "ineluctable modality of the visible",
    "Une oasis d'horreur dans un désert d'ennui",
    "E pur si muov",
    "Lasciate ogne speranza, voi ch'intrate",
    "∀u1…∀uk[∀x∃!yφ(x,y,û) → ∀w∃v∀r(r∈v ≡ ∃s(s∈w & φx,y,û[s,r,û]))]",
    "Puotsu mangiari u vitru, nun mi fa mali",
    "Jau sai mangiar vaider, senza che quai fa donn a mai",
    "Pot să mănânc sticlă și ea nu mă rănește",
    "‽⅏⅋℺℧℣",
    "Mi povas manĝi vitron, ĝi ne damaĝas min",
    "Mý a yl dybry gwéder hag éf ny wra ow ankenya",
    "Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi",
    "Foddym gee glonney agh cha jean eh gortaghey mee",
    "᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ",
    "Con·iccim ithi nglano. Ním·géna",
    "⚔☢☭࿗☮࿘☭☣",
    "Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom",
    "Ithim-sa gloine agus ní miste damh é",
    "S urrainn dhomh gloinne ithe; cha ghoirtich i mi",
    "ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ",
    "Ic mæg glæs eotan ond hit ne hearmiað me",
    "Ich canne glas eten and hit hirtiþ me nouȝt",
    "I can eat glass and it doesn't hurt me",
    "aɪ kæn iːt glɑːs ænd ɪt dɐz nɒt hɜːt mi",
    "⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍",
    "Mi kian niam glas han i neba hot mi",
    "Ah can eat gless, it disnae hurt us",
    "𐌼𐌰𐌲 𐌲𐌻𐌴𐍃 𐌹̈𐍄𐌰𐌽, 𐌽𐌹 𐌼𐌹𐍃 𐍅𐌿 𐌽𐌳𐌰𐌽 𐌱𐍂𐌹𐌲𐌲𐌹𐌸",
    "ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨ",
    "Ek get etið gler án þess að verða sár",
    "Eg kan eta glas utan å skada meg",
    "Jeg kan spise glass uten å skade meg",
    "Eg kann eta glas, skaðaleysur",
    "Ég get etið gler án þess að meiða mig",
    "𝐸 = 𝑚𝑐²",
    "Jag kan äta glas utan att skada mig",
    "Jeg kan spise glas, det gør ikke ondt på mig",
    "㎚㎛㎜㎝㎞㎟㎠㎡㎢㎣㎤㎥㎦㎕㎖㎗㎘㏄㎰㎱㎲㎳㎍㎎㎏㎅㎆㏔㎇㎐㎑㎒㎓㎔㎮㎯",
    "Æ ka æe glass uhen at det go mæ naue",
    // FIXME this one
    "က္ယ္ဝန္တော္၊က္ယ္ဝန္မ မ္ယက္စားနုိင္သည္။ ၎က္ရောင္ ထိခုိက္မ္ဟု မရ္ဟိပာ။",
    "ကျွန်တော် ကျွန်မ မှန်စားနိုင်တယ်။ ၎င်းကြောင့် ထိခိုက်မှုမရှိပါ။ ",
    "Tôi có thể ăn thủy tinh mà không hại gì",
    "些 𣎏 世 咹 水 晶 𦓡 空 𣎏 害",
    "ខ្ញុំអាចញុំកញ្ចក់បាន ដោយគ្មានបញ្ហា",
    "ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ",
    "ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็",
    "Би шил идэй чадна, надад хортой би",
    "ᠪᠢ ᠰᠢᠯᠢ ᠢᠳᠡᠶᠦ ᠴᠢᠳᠠᠨᠠ ᠂ ᠨᠠᠳᠤᠷ ᠬᠣᠤᠷᠠᠳᠠᠢ ᠪᠢᠰ",
    "म काँच खान सक्छू र मलाई केहि नी हुन्न्",
    "ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད",
    "我能吞下玻璃而不伤身体",
    "我能吞下玻璃而不傷身體",
    "Góa ē-tàng chia̍h po-lê, mā bē tio̍h-siong",
    "私はガラスを食べられますそれは私を傷つけません",
    "나는 유리를 먹을 수 있어요. 그래도 아프지 않아",
    "Mi save kakae glas, hemi no save katem mi",
    "Hiki iaʻu ke ʻai i ke aniani; ʻaʻole nō lā au e ʻeha",
    "E koʻana e kai i te karahi, mea ʻā, ʻaʻe hauhau",
    "ᐊᓕᒍᖅ ᓂᕆᔭᕌᖓᒃᑯ ᓱᕋᙱᑦᑐᓐᓇᖅᑐ",
    "Naika məkmək kakshət labutay, pi weyk ukuk munk-sik nay",
    "Tsésǫʼ yishą́ągo bííníshghah dóó doo shił neezgai da",
    "mi kakne le nu citka le blaci .iku'i le se go'i na xrani m",
    "Ljœr ye caudran créneþ ý jor cẃran",
    "Ik kin glês ite, it docht me net sear",
    "Ik kan glas eten, het doet mĳ geen kwaad",
    "Iech ken glaas èèse, mer 't deet miech jing pieng",
    "Ek kan glas eet, maar dit doen my nie skade nie",
    "Ech kan Glas iessen, daat deet mir nët wei",
    "Ich kann Glas essen, ohne mir zu schaden",
    "Ich kann Glas verkasematuckeln, ohne dattet mich wat jucken tut",
    "Isch kann Jlaas kimmeln, uuhne datt mich datt weh dääd",
    "Ich koann Gloos assn und doas dudd merr ni wii",
    "Мен шиша ейишим мумкин, аммо у менга зарар келтирмайди",
    "আমি কাঁচ খেতে পারি, তাতে আমার কোনো ক্ষতি হয় না",
    "मी काच खाऊ शकतो, मला ते दुखत नाही",
    "ನನಗೆ ಹಾನಿ ಆಗದೆ, ನಾನು ಗಜನ್ನು ತಿನಬಹು",
    "मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती",
    "എനിക്ക് ഗ്ലാസ് തിന്നാം. അതെന്നെ വേദനിപ്പിക്കില്ല",
    "நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது",
    "నేను గాజు తినగలను మరియు అలా చేసినా నాకు ఏమి ఇబ్బంది లే",
    "මට වීදුරු කෑමට හැකියි. එයින් මට කිසි හානියක් සිදු නොවේ",
    "میں کانچ کھا سکتا ہوں اور مجھے تکلیف نہیں ہوتی",
    "زه شيشه خوړلې شم، هغه ما نه خوږو",
    ".من می توانم بدونِ احساس درد شيشه بخور",
    "أنا قادر على أكل الزجاج و هذا لا يؤلمني",
    "Nista' niekol il-ħġieġ u ma jagħmilli xejn",
    "אני יכול לאכול זכוכית וזה לא מזיק לי",
    "איך קען עסן גלאָז און עס טוט מיר נישט װײ",
    "Metumi awe tumpan, ɜnyɜ me hwee",
    "Iech konn glaasch voschbachteln ohne dass es mir ebbs daun doun dud",
    "'sch kann Glos essn, ohne dass'sch mer wehtue",
    "Isch konn Glass fresse ohne dasses mer ebbes ausmache dud",
    "I kå Glas frässa, ond des macht mr nix",
    "I ka glas eassa, ohne dass mar weh tuat",
    "I koh Glos esa, und es duard ma ned wei",
    "卂丂爪𝑜Ⓓє𝓊Ⓢ в𝔢𝓁Įαlнᗩs𝓉𝐮ℝ 𝐍чคⓇ𝓛Ａт卄ＯｔＥᵖ 𝔴ᗝⓣ𝓐ⓝ 𝐧ίｇ𝕘𝐔尺𝓪ᵗ𝕙 𝔻ĤＯ𝔩ᵉ𝔰 卂žᵃ𝓣ĤỖ𝔱𝓗 Ť𝔦ℕ𝔻Ａℓ๏Ş ᛕＡĐ𝐈𝓽ħ",
    "I kaun Gloos essen, es tuat ma ned weh",
    "Ich chan Glaas ässe, das schadt mir nöd",
    "Ech cha Glâs ässe, das schadt mer ned",
    "Ja mahu jeści škło, jano mne ne škodzić",
    "Я можу їсти скло, і воно мені не зашкодить",
    "Мога да ям стъкло, то не ми вреди",
    "მინას ვჭამ და არა მტკივა",
    "Կրնամ ապակի ուտել և ինծի անհանգիստ չըներ",
    "Unë mund të ha qelq dhe nuk më gjen gjë",
    "Cam yiyebilirim, bana zararı dokunmaz",
    "جام ييه بلورم بڭا ضررى طوقونم",
    "Алам да бар, пыяла, әмма бу ранит мине",
    "Men shisha yeyishim mumkin, ammo u menga zarar keltirmaydi",
    "Inā iya taunar gilāshi kuma in gamā lāfiyā",
    "إِنا إِىَ تَونَر غِلَاشِ كُمَ إِن غَمَا لَافِىَ",
    "Mo lè je̩ dígí, kò ní pa mí lára",
    "Nakokí kolíya biténi bya milungi, ekosála ngáí mabé tɛ́",
    "Naweza kula bilauri na sikunyui",
    "Saya boleh makan kaca dan ia tidak mencederakan saya",
    "Kaya kong kumain nang bubog at hindi ako masaktan",
    "Siña yo' chumocho krestat, ti ha na'lalamen yo'",
    "Au rawa ni kana iloilo, ia au sega ni vakacacani kina",
    "Aku isa mangan beling tanpa lara",
    "ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗᛋᚳᛖᚪᛚ᛫ᚦᛖᚪᚻ᛫ᛗᚪᚾᚾᚪ᛫ᚷᛖᚻᚹᛦᛚᚳ᛫ᛗᛁᚳᛚᚢᚾ᛫ᚻᛦᛏ᛫ᛞᚫᛚᚪᚾᚷᛁᚠ᛫ᚻᛖ᛫ᚹᛁᛚᛖ᛫ᚠᚩᚱ᛫ᛞᚱᛁᚻᛏᚾᛖ᛫ᛞᚩᛗᛖᛋ᛫ᚻᛚᛇᛏᚪᚾ᛬",
    "An preost wes on leoden, Laȝamon was ihoten He wes Leovenaðes sone -- liðe him be Drihten. He wonede at Ernleȝe at æðelen are chirechen, Uppen Sevarne staþe, sel þar him þuhte, Onfest Radestone, þer he bock radde.",
    "Sîne klâwen durh die wolken sint geslagen, er stîget ûf mit grôzer kraft, ich sih in grâwen tägelîch als er wil tagen, den tac, der im geselleschaft erwenden wil, dem werden man, den ich mit sorgen în verliez. ich bringe in hinnen, ob ich kan. sîn vil manegiu tugent michz leisten hiez.",
    "Τη γλώσσα μου έδωσαν ελληνική το σπίτι φτωχικό στις αμμουδιές του Ομήρου. Μονάχη έγνοια η γλώσσα μου στις αμμουδιές του Ομήρου. από το Άξιον Εστί του Οδυσσέα Ελύτη",
    "На берегу пустынных волн Стоял он, дум великих полн, И вдаль глядел. Пред ним широко Река неслася; бедный чёлн По ней стремился одиноко. По мшистым, топким берегам Чернели избы здесь и там, Приют убогого чухонца; И лес, неведомый лучам В тумане спрятанного солнца, Кругом шумел.",
    "ვეპხის ტყაოსანი შოთა რუსთაველი ღმერთსი შემვედრე, ნუთუ კვლა დამხსნას სოფლისა შრომასა, ცეცხლს, წყალსა და მიწასა, ჰაერთა თანა მრომასა; მომცნეს ფრთენი და აღვფრინდე, მივჰხვდე მას ჩემსა ნდომასა, დღისით და ღამით ვჰხედვიდე მზისა ელვათა კრთომაასა",
    "யாமறிந்த மொழிகளிலே தமிழ்மொழி போல் இனிதாவது எங்கும் காணோம், பாமரராய் விலங்குகளாய், உலகனைத்தும் இகழ்ச்சிசொலப் பான்மை கெட்டு, நாமமது தமிழரெனக் கொண்டு இங்கு வாழ்ந்திடுதல் நன்றோ? சொல்லீர்! தேமதுரத் தமிழோசை உலகமெலாம் பரவும்வகை செய்தல் வேண்டும்.",
    NULL
  };
  const char** s;
  const int steps[] = { 0, 0x10040, 0x20110, 0x120, 0x12020, };
  const int starts[] = { 0, 0x10101, 0x004000, 0x000040, 0x400040, };

  // this demo is completely meaningless outside UTF-8 mode
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  size_t i;
  const size_t screens = sizeof(steps) / sizeof(*steps);
  struct ncplane* n = notcurses_stdplane(nc);
  bool initial_scroll = ncplane_scrolling_p(n);
  ncplane_set_scrolling(n, true);
  ncplane_erase(n);
  (void)startns; // FIXME integrate
  for(i = 0 ; i < screens ; ++i){
    uint32_t key = NCKEY_INVALID;
    nccell c;
    struct timespec screenend;
    clock_gettime(CLOCK_MONOTONIC, &screenend);
    ns_to_timespec(timespec_to_ns(&screenend) + timespec_to_ns(&demodelay), &screenend);
    do{ // (re)draw a screen
      const int start = starts[i];
      int step = steps[i];
      nccell_init(&c);
      unsigned maxy, maxx;
      ncplane_dim_yx(n, &maxy, &maxx); // might resize
      uint32_t rgb = start;
      unsigned bytes_out = 0;
      unsigned egcs_out = 0;
      unsigned cols_out = 0;
      unsigned y = 1;
      unsigned x = 0;
      if(ncplane_cursor_move_yx(n, y, x)){
        return -1;
      }
      ncplane_set_bg_rgb8(n, 20, 20, 20);
      do{ // we fill up the screen, however large, bouncing around our strtable
        s = strs + rand() % ((sizeof(strs) / sizeof(*strs)) - 1);
        if(dostring(n, s, rgb, maxy, maxx, &egcs_out, &cols_out, &bytes_out)){
          return -1;
        }
        rgb += step;
        ncplane_cursor_yx(n, &y, &x);
      }while(y < (maxy - 1) || x < (maxx - 2));
      struct ncplane* math = mathplane(nc);
      if(math == NULL){
        return -1;
      }
      struct ncplane_options nopts = {
        .y = 2,
        .x = 4,
        .rows = 7,
        .cols = 57,
      };
      struct ncplane* mess = ncplane_create(n, &nopts);
      if(mess == NULL){
        ncplane_destroy(math);
        return -1;
      }
      if(message(mess, maxy, maxx, i, sizeof(steps) / sizeof(*steps),
                 bytes_out, egcs_out, cols_out)){
        ncplane_destroy(math); ncplane_destroy(mess);
        return -1;
      }
      int err;
      if( (err = demo_render(nc)) ){
        ncplane_destroy(math); ncplane_destroy(mess);
        return err;
      }
      if(i){
        uint64_t delay = timespec_to_ns(&demodelay);
        delay /= screens;
        struct timespec tv;
        if(delay > NANOSECS_IN_SEC){
          ns_to_timespec(NANOSECS_IN_SEC, &tv);
        }else{
          ns_to_timespec(delay, &tv);
        }
        ncplane_fadein(n, &tv, demo_fader, NULL);
      }

      struct worm_ctx wctx;
      if( (err = init_worms(&wctx, maxy, maxx)) ){
        return err;
      }
      struct timespec cur;
      do{
        if( (err = worm_move(nc, &wctx, maxy, maxx)) ){
          break;
        }
        struct timespec ts;
        ns_to_timespec(timespec_to_ns(&demodelay) / 10000, &ts);
        key = demo_getc(nc, &ts, NULL);
        clock_gettime(CLOCK_MONOTONIC, &cur);
        if(timespec_to_ns(&screenend) < timespec_to_ns(&cur)){
          break;
        }
      }while(key != NCKEY_RESIZE);
      free(wctx.worms);

      ncplane_destroy(mess);
      ncplane_destroy(math);
      if(err){
        return err;
      }
      if(key == NCKEY_RESIZE){
        DEMO_RENDER(nc);
      }
    }while(key == NCKEY_RESIZE);
  }
  ncplane_set_scrolling(n, initial_scroll);
  return 0;
}
