#include <locale.h>
#include <notcurses/notcurses.h>

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options nops = {};
  nops.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&nops, stdout);
  struct ncplane* n = notcurses_stdplane(nc);
  const int y = 10;
  ncplane_putstr_aligned(n, y + 0, NCALIGN_CENTER, "Pack my box with five dozen liquor jugs.");
  ncplane_putstr_aligned(n, y + 1, NCALIGN_CENTER, "˙sɓnɾ ɹonbıl uǝzop ǝʌıɟ ɥʇıʍ xoq ʎɯ ʞɔɐԀ");
  ncplane_putstr_aligned(n, y + 2, NCALIGN_CENTER, ".ꙅǫuꞁ ɿoupi| ᴎɘƹob ɘviᎸ ʜƚiw xod ʏm ʞɔɒꟼ");
  ncplane_putstr_aligned(n, y + 3, NCALIGN_CENTER, "P̸̯̼͙̻̲͚̜͚͈̩̎͠a̶̯̳̱̟͚͇̩̯̬͂̒̒̌̅͊̽̿͗̈́͘͝ͅć̸̮̦̩̭͓̫̟̹̆͂͒̓͆̈̅̀͐̿̚ͅk̶̡̻̜̼̙͍̥̗̯̠̜͓̪̽ ̷̮͚̺͎̗̂̈́̿̑͝m̴̩͍̺̟͓̼͓͇̟̙͂̏̈́͆̎́̐̐̽̕̚͘͜ͅy̵̧̻̗̦̯̱̬̤͈̦̺͗̓͛ ̵̣̤͕͙̟͛͌͑̉͘͝b̵̨̡̨̯̞̘͕̰̙̬̳͇̮͖̹͗o̶̻̫͑̎͑̽̈́̚̚̕ͅx̵̢̦̗̳̝̻̗̟̘̻̼͚̰̓̇̓͛̀ ̸͈̜͙́̍͂̅̃̿͘w̵̧̪̻̤̮̑͊͌̈́͋́̔͂̑̌͑͋̇͂ḯ̴̺͛̒̔̏́̅̓̾̔̊̆͗͠͝t̷̢̛̟͉͕̗̙̭̖͈̼̂̎́͌͜͝h̸̢̨̜̤̗͎̳̖͙̺̹̭̘̞̀̀̓̊̐̀͐̈́̀̿͆̔̄͝ͅ ̴̹̙̜̥͕͖͑́͛̈́̄̈́̿̕f̶̡̢̳̗͉̩͖̹͚̗̩̰͖̀̂͗͌̑i̴̧̝̰͎͕̣͓͓̋͒̇̀̾͐̃̚̕v̵̛̠̩̪̻̟͕̭͕̗̲̼̽͗͐̄̈́̾͂̍̔̎͌̌͘͝e̵̢̤͇̪̻͚̜͉̻͉̝͙͗̀̃̐͋̌̋̌̈́̅̈̌̉͘͜ ̶̡̢̭͙̤͑̑͂̐͛̑̍͑͌̀͛d̴̨̧͍̫͔͔̫̻̗̙͖̞̱͆̒͂̈̐͑̕̚͠͝ó̸̰̠̦̦̞̼̘͔̥͎͕̦̯̑̀̇̈́̎̄͘z̴̡̧̡̦̦̙̞̪̣̤͕̫̳̈̉͌̃͌͛̀͌̎̃̌͒͜͝ȩ̷̻͖̥̬̹̖̫͛͐̍̂̾̀͑͊̎̀̊̏̕ͅn̵̢̢̧͚̜͉̯̲͕̒͊̒͌͋͗̓́͂͝ ̵̡̧̨̛̛͓͚̘̺̲̺̻̻̫̾̄̒̑̄̄̏̇̍̽͜l̴̗̩͍̰̇i̴̡̭̳͉̘̩͚̽̏̿̈̔́̂̈́̊͝q̷̫͚̌̅̈́̓̐̎ů̶̢͈̪͔̅̓̀̓̓̈͆̍͋͋̉͝͠ǫ̵̻͠r̵̰̯̠̟̬͖̳͔̚ͅ ̶̡̣̭̥̻̭͙͎̰̜̥̜̊j̷̧̡̟̝̼̞̭͙͈̘̇̾̽͊̄̈̍͗͒͑͜u̷̡̪̤̖̣̰͈̽̀̚͜g̸̨̢̧̳̙̝̠̩̜̻͙̘̪̞̈́͐̈́̇̆̎̈ͅs̶̫͑͂̂͛̋̇̅̒͝.̴͈̖̮̪̮͓̹̈̐̇̓̇͝ ̵̭̤͐͗̇̽̎́͆͋͌͜");
  ncplane_putstr_aligned(n, y + 4, NCALIGN_CENTER, "Ⓟⓐⓒⓚ ⓜⓨ ⓑⓞⓧ ⓦⓘⓣⓗ ⓕⓘⓥⓔ ⓓⓞⓩⓔⓝ ⓛⓘⓠⓤⓞⓡ ⓙⓤⓖⓢ ");
  ncplane_putstr_aligned(n, y + 5, NCALIGN_CENTER, "φąçҟ ʍվ ҍօ× աìէհ ƒìѵҽ ժօՀҽղ Ӏìզմօɾ ʝմցʂ.");
  ncplane_putstr_aligned(n, y + 6, NCALIGN_CENTER, "P⃞a⃞c⃞k⃞m⃞y⃞b⃞o⃞x⃞w⃞i⃞t⃞h⃞f⃞i⃞v⃞e⃞d⃞o⃞z⃞e⃞n⃞l⃞i⃞q⃞u⃞o⃞r⃞j⃞u⃞g⃞s⃞.⃞");
  ncplane_putstr_aligned(n, y + 7, NCALIGN_CENTER, "P⃣a⃣c⃣k⃣m⃣y⃣b⃣o⃣x⃣w⃣i⃣t⃣h⃣f⃣i⃣v⃣e⃣d⃣o⃣z⃣e⃣n⃣l⃣i⃣q⃣u⃣o⃣r⃣j⃣u⃣g⃣s⃣.⃣");
  ncplane_putstr_aligned(n, y + 8, NCALIGN_CENTER, "ᴘᴀᴄᴋ ᴍʏ ʙᴏx ᴡɪᴛʜ ꜰɪᴠᴇ ᴅᴏᴢᴇɴ ʟɪQᴜᴏʀ ᴊᴜɢꜱ.");
  ncplane_putstr_aligned(n, y + 9, NCALIGN_CENTER, "🅝🅔🅖🅐🅣🅘🅥🅔 🅒🅘🅡🅒🅛🅔🅢 🅐🅡🅔 🅐🅛🅢🅞 🅐🅥🅐🅘🅛🅐🅑🅛🅔");
  ncplane_putstr_aligned(n, y + 10, NCALIGN_CENTER, "🄴 🅂 🄲 🄷 🄴 🅆  🄲 🄸 🅁 🄲 🄻 🄴 🅂  🄶 🄴 🅃  🅂 🅀 🅄 🄰 🅁 🄴 🅂");
  ncplane_putstr_aligned(n, y + 11, NCALIGN_CENTER, "🅰 🅱🅴🅰🆄🆃🅸🅵🆄🅻 🆄🆂🅴 🅾🅵 🅽🅴🅶🅰🆃🅸🆅🅴 🆂🆀🆄🅰🆁🅴🆂");
  notcurses_render(nc);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
