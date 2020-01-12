#include <cstdlib>
#include <locale.h>
#include <unistd.h>
#include <notcurses.h>

int mathtext(struct notcurses* nc){
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  const int HEIGHT = 9;
  const int WIDTH = dimx;
  struct ncplane* n = ncplane_new(nc, HEIGHT, WIDTH, dimy - HEIGHT - 1, dimx - WIDTH - 1, NULL);
  if(n){
    struct ncplane* stdn = notcurses_stdplane(nc);
    ncplane_set_fg(n, 0xffffff);
    ncplane_set_bg(n, 0x008000);
    ncplane_printf_aligned(n, 0, NCALIGN_RIGHT, "∮E⋅da=Q,n→∞,∑f(i)=∏g(i)⎧⎡⎛┌─────┐⎞⎤⎫");
    ncplane_printf_aligned(n, 1, NCALIGN_RIGHT, "⎪⎢⎜│a²+b³ ⎟⎥⎪");
    ncplane_printf_aligned(n, 2, NCALIGN_RIGHT, "∀x∈ℝ:⌈x⌉=−⌊−x⌋,α∧¬β=¬(¬α∨β)⎪⎢⎜│───── ⎟⎥⎪");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "⎪⎢⎜⎷ c₈   ⎟⎥⎪");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, "ℕ⊆ℕ₀⊂ℤ⊂ℚ⊂ℝ⊂ℂ(z̄=ℜ(z)−ℑ(z)⋅𝑖)⎨⎢⎜       ⎟⎥⎬");
    ncplane_printf_aligned(n, 5, NCALIGN_RIGHT, "⎪⎢⎜ ∞     ⎟⎥⎪");
    ncplane_printf_aligned(n, 6, NCALIGN_RIGHT, "⊥<a≠b≡c≤d≪⊤⇒(⟦A⟧⇔⟪B⟫)⎪⎢⎜ ⎲     ⎟⎥⎪");
    ncplane_printf_aligned(n, 7, NCALIGN_RIGHT, "⎪⎢⎜ ⎳aⁱ-bⁱ⎟⎥⎪");
    ncplane_printf_aligned(n, 8, NCALIGN_RIGHT, "2H₂+O₂⇌2H₂O,R=4.7kΩ,⌀200µm⎩⎣⎝i=1    ⎠⎦⎭");
  }
  return 0;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  const char c[] =
"Jegkanspiseglassutenåskademeg"
"Egkannetaglasskaðaleysur"
"Éggetetiðgleránþessaðmeiðamig"
"𝐸=𝑚𝑐²"
"Jagkanätaglasutanattskadamig"
"Jegkanspiseglasdetgørikkeondtpåmig"
"㎚㎛㎜㎝㎞㎟㎠㎡㎢㎣㎤㎥㎦㎕㎖㎗㎘㏄㎰㎱㎲㎳㎍㎎㎏㎅㎆㏔㎇㎐㎑㎒㎓㎔㎮㎯"
"Ækaæeglassuhenatdetgomænaue"
"က္ယ္ဝန္တော၊က္ယ္ဝန္မမ္ယက္စားနုိင္သည္။၎က္ရောင္ထိခုိက္မ္ဟုမရ္ဟိပာ။"
"ကျွန်တောကျွန်မမှန်စားနိုင်တယ်။၎င်းကြေင့်ထိခိုက်မှုမရှိပါ။"
"Tôicóthểănthủytinhmàkhônghạigì"
"些𣎏世咹水晶𦓡空𣎏害"
"ខ្ញុំអាចញុំកញ្ចក់បានដោយគ្មានបញ្ហា"
"ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ"
"ฉันกินกระจกได้แต่มันไม่ทำให้ฉันเจ็"
"Бишилидэйчаднанададхортойби"
"ᠪᠢᠰᠢᠯᠢᠢᠳᠡᠶᠦᠴᠢᠳᠠᠨᠠ᠂ᠨᠠᠳᠤᠷᠬᠣᠤᠷᠠᠳᠠᠢᠪᠢᠰ"
"मकाँचखानसक्छूरमलाईकेहिनीहुन्न्"
"ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད"
"我能吞下玻璃而不伤身体"
"我能吞下玻璃而不傷身體"
"Góaē-tàngchia̍hpo-lêmābētio̍h-siong"
"私はガラスを食べられますそれは私を傷つけません"
"나는유리를먹을수있어요.그래도아프지않아"
"Misavekakaeglasheminosavekatemmi"
"Hikiiaʻukeʻaiikeaniani;ʻaʻolenōlāaueʻeha"
"Ekoʻanaekaiitekarahimeaʻāʻaʻehauhau"
"ᐊᓕᒍᖅᓂᕆᔭᕌᖓᒃᑯᓱᕋᙱᑦᑐᓐᓇᖅᑐ";
  struct ncplane* n = notcurses_stdplane(nc);
  int y, dimy;
  notcurses_term_dim_yx(nc, &dimy, nullptr);
  do{
    ncplane_putstr(n, c);
    ncplane_cursor_yx(n, &y, nullptr);
  }while(y < dimy);
  if(mathtext(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  notcurses_stop(nc);
  return 0;
}
