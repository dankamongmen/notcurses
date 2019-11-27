#include <ctype.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "demo.h"

// Much of this text comes from http://kermitproject.org/utf8.html
int widecolor_demo(struct notcurses* nc){
  static const char* strs[] = {
    "Война и мир",
    "Бра́тья Карама́зовы",
    "Час сэканд-хэнд",
    "ஸீரோ டிகிரி",
    "Tonio Kröger",
    /*"بين القصرين",
    "قصر الشوق",
    "السكرية",*/
    /* "三体",
    "血的神话公元年湖南道县文革大屠杀纪实",
    "三国演义",
    "紅樓夢",
    "Hónglóumèng",
    "红楼梦",
    "महाभारतम्",
    "Mahābhāratam",
    " रामायणम्",*/
    "Rāmāyaṇam",
    /* "القرآن",
    "תּוֹרָה",
    "תָּנָ״ךְ",*/
    "Osudy dobrého vojáka Švejka za světové války",
    "Σίβνλλα τί ϴέλεις; respondebat illa: άπο ϴανεΐν ϴέλω",
    /*
    "① На всей земле был один язык и одно наречие.",
    "② А кад отидоше од истока, нађоше равницу у земљи сенарској, и населише се онде.",
    "③ І сказалі адно аднаму: наробім цэглы і абпалім агнём. І стала ў іх цэгла замест камянёў, а земляная смала замест вапны.",
    "④ І сказали вони: Тож місто збудуймо собі, та башту, а вершина її аж до неба. І вчинімо для себе ймення, щоб ми не розпорошилися по поверхні всієї землі.",
    "⑤ Господ слезе да ги види градот и кулата, што луѓето ги градеа.",
    "⑥ И҆ речѐ гдⷭ҇ь: сѐ, ро́дъ є҆ди́нъ, и҆ ѹ҆стнѣ̀ є҆ди҄нѣ всѣ́хъ, и҆ сїѐ нача́ша твори́ти: и҆ нн҃ѣ не ѡ҆скꙋдѣ́ютъ ѿ ни́хъ всѧ҄, є҆ли҄ка а́҆ще восхотѧ́тъ твори́ти.",
    "⑦ Ⱂⱃⰻⰻⰴⱑⱅⰵ ⰺ ⰺⰸⱎⰵⰴⱎⰵ ⱄⰿⱑⱄⰻⰿⱏ ⰺⰿⱏ ⱅⱆ ⱔⰸⱏⰹⰽⰻ ⰺⱈⱏ · ⰴⰰ ⱀⰵ ⱆⱄⰾⱏⰹⱎⰰⱅⱏ ⰽⱁⰶⰴⱁ ⰴⱃⱆⰳⰰ ⱄⰲⱁⰵⰳⱁ ⁖⸏",
    "काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम्",
    "kācaṃ śaknomyattum; nopahinasti mām",
    "ὕαλον ϕαγεῖν δύναμαι· τοῦτο οὔ με βλάπτει",
    "Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα",
    "Vitrum edere possum; mihi non nocet",
    "🚬🌿💉💊☢☣🔫💣⚔🤜🤛🧠🦹🤺🏋️,🦔🐧🐣🦆🦢🦜🦉🐊🐸🦕 🦖🐬🐙🦂🦠🦀",
    "Je puis mangier del voirre. Ne me nuit",
    "Je peux manger du verre, ça ne me fait pas mal",
    "Pòdi manjar de veire, me nafrariá pas",
    "J'peux manger d'la vitre, ça m'fa pas mal",
    "Dji pou magnî do vêre, çoula m' freut nén må",
    "Ch'peux mingi du verre, cha m'foé mie n'ma",
    "Mwen kap manje vè, li pa blese'm",
    "Kristala jan dezaket, ez dit minik ematen",
    "Puc menjar vidre, que no em fa mal",
    "Puedo comer vidrio, no me hace daño",
    "Puedo minchar beire, no me'n fa mal",
    "Eu podo xantar cristais e non cortarme",
    "Posso comer vidro, não me faz mal",
    "Posso comer vidro, não me machuca",
    "M' podê cumê vidru, ca ta maguâ-m'",
    "Ami por kome glas anto e no ta hasimi daño",
    "Posso mangiare il vetro e non mi fa male",
    "Sôn bôn de magnà el véder, el me fa minga mal",
    "Me posso magna' er vetro, e nun me fa male",
    "M' pozz magna' o'vetr, e nun m' fa mal",
    "Mi posso magnare el vetro, no'l me fa mae",
    "Pòsso mangiâ o veddro e o no me fà mâ",
    "Puotsu mangiari u vitru, nun mi fa mali",
    "Jau sai mangiar vaider, senza che quai fa donn a mai",
    "Pot să mănânc sticlă și ea nu mă rănește",
    "Mi povas manĝi vitron, ĝi ne damaĝas min",
    "Mý a yl dybry gwéder hag éf ny wra ow ankenya",
    "Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi",
    "Foddym gee glonney agh cha jean eh gortaghey mee",
    "᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ",
    "Con·iccim ithi nglano. Ním·géna",
    "🗽🏴☭࿗☮࿘☭🏴🗽",
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
    "Jag kan äta glas utan att skada mig",
    "Jeg kan spise glas, det gør ikke ondt på mig",
    "Æ ka æe glass uhen at det go mæ naue",
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
    "私はガラスを食べられます。それは私を傷つけません",
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
    // "میں کانچ کھا سکتا ہوں اور مجھے تکلیف نہیں ہوتی",
    // "زه شيشه خوړلې شم، هغه ما نه خوږو",
    // ".من می توانم بدونِ احساس درد شيشه بخور",
    // "أنا قادر على أكل الزجاج و هذا لا يؤلمني",
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
    "I kaun Gloos essen, es tuat ma ned weh",
    "Ich chan Glaas ässe, das schadt mir nöd",
    "Ech cha Glâs ässe, das schadt mer ned",
    "Meg tudom enni az üveget, nem lesz tőle bajom",
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
    */
    NULL
  };
  const char** s;
  int count = notcurses_palette_size(nc);
  const int steps[] = { 128, 64, 16, 1, };
  const int starts[] = { 0, 16, 62, 128, };

  struct ncplane* n = notcurses_stdplane(nc);
  size_t i;
  for(i = 0 ; i < sizeof(steps) / sizeof(*steps) ; ++i){
  ncplane_erase(n);
    const int start = starts[i];
    const int step = steps[i];
    //do{
      int y, x, maxy, maxx;
      ncplane_dimyx(n, &maxy, &maxx);
      int rgb = start;
      if(ncplane_cursor_move_yx(n, 0, 0)){
        return -1;
      }
      y = 0;
      x = 0;
      do{ // we fill up the entire screen, however large
        s = strs;
        for(s = strs ; *s ; ++s){
          cell wch = CELL_TRIVIAL_INITIALIZER;
          cell_set_style(&wch, WA_NORMAL);
          cell_set_fg(&wch, cell_rgb_red(rgb), 255 - cell_rgb_green(rgb),
                      cell_rgb_blue(rgb));
          cell_set_bg(&wch, 64, 64, 64);
          size_t idx = 0;
          ncplane_cursor_yx(n, &y, &x);
          fprintf(stderr, "%02d %s\n", y, *s);
          while((*s)[idx]){
            if(y >= maxy || x >= maxx){
              break;
            }
            if(isspace((*s)[idx])){
              ++idx;
              continue;
            }
            // cell_load frees the previous contents
            int ulen = cell_load(n, &wch, &(*s)[idx]);
            if(ulen < 0){
              return -1;
            }
            if(ncplane_putc(n, &wch) < 0){
              break;
            }
            ncplane_cursor_yx(n, &y, &x);
            if((rgb += step) >= count){
              rgb = 1;
            }
            idx += ulen;
          }
          cell_release(n, &wch);
        }
      }while(y < maxy && x < maxx);
      ncplane_fg_rgb8(n, 255, 255, 255);
      // ncplane_set_style(n, WA_BOLD);
      ncplane_cursor_move_yx(n, 2, 2);
      ncplane_printf(n, " %dx%d (%d/%d) ", maxx, maxy, i, sizeof(steps) / sizeof(*steps));
      ncplane_cursor_move_yx(n, 3, 2);
      ncplane_set_style(n, WA_NORMAL);
      ncplane_fg_rgb8(n, 200, 20, 200);
      ncplane_putstr(n, " 🔥wide chars, multiple colors, resize awareness…🔥 ");
      if(notcurses_render(nc)){
        return -1;
      }
      /*if(i){ FIXME
        fadein(w, count, palette, FADE_MILLISECONDS);
      }
      do{
        key = wgetch(w);
      }while(key == ERR);
      */
      sleep(1); // FIXME
    //}while(key == KEY_RESIZE);
  }
  return 0;
}
