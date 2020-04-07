#include <cstdlib>
#include <clocale>
#include <memory>
#include <unistd.h>
#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

int mathtext([[maybe_unused]] NotCurses& nc, std::shared_ptr<Plane> n){
  if(n){
    n->set_fg(0xffffff);
    n->set_bg(0x008000);
    n->printf(0, NCAlign::Right, "∮E⋅da=Q,n→∞,∑f(i)=∏g(i)⎧⎡⎛┌─────┐⎞⎤⎫");
    n->printf(1, NCAlign::Right, "⎪⎢⎜│a²+b³ ⎟⎥⎪");
    n->printf(2, NCAlign::Right, "∀x∈ℝ:⌈x⌉=−⌊−x⌋,α∧¬β=¬(¬α∨β)⎪⎢⎜│───── ⎟⎥⎪");
    n->printf(3, NCAlign::Right, "⎪⎢⎜⎷ c₈   ⎟⎥⎪");
    n->printf(4, NCAlign::Right, "ℕ⊆ℕ₀⊂ℤ⊂ℚ⊂ℝ⊂ℂ(z̄=ℜ(z)−ℑ(z)⋅𝑖)⎨⎢⎜       ⎟⎥⎬");
    n->printf(5, NCAlign::Right, "⎪⎢⎜ ∞     ⎟⎥⎪");
    n->printf(6, NCAlign::Right, "⊥<a≠b≡c≤d≪⊤⇒(⟦A⟧⇔⟪B⟫)⎪⎢⎜ ⎲     ⎟⎥⎪");
    n->printf(7, NCAlign::Right, "⎪⎢⎜ ⎳aⁱ-bⁱ⎟⎥⎪");
    n->printf(8, NCAlign::Right, "2H₂+O₂⇌2H₂O,R=4.7kΩ,⌀200µm⎩⎣⎝i=1    ⎠⎦⎭");
  }
  return 0;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  NotCurses::default_notcurses_options.inhibit_alternate_screen = true;
  NotCurses nc;

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
  std::unique_ptr<Plane> nstd(nc.get_stdplane());
  int y, dimy, dimx;
  nc.get_term_dim(&dimy, &dimx);
  nstd->set_scrolling(true);
  do{
    nstd->putstr(c);
    nstd->get_cursor_yx(&y, nullptr);
  }while(y < dimy);
  const int HEIGHT = 9;
  const int WIDTH = dimx;
  std::shared_ptr<Plane> n = std::make_shared<Plane>(HEIGHT, WIDTH, dimy - HEIGHT - 1, dimx - WIDTH - 1);
  if(mathtext(nc, n)){
    return EXIT_FAILURE;
  }
  nc.render();
  return 0;
}
