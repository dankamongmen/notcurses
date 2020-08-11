// These defined macros can't be handled by bindgen yet, see:
//  - https://github.com/rust-lang/rust-bindgen/issues/316
//  - https://github.com/jethrogb/rust-cexpr/pull/15

pub const fn suppuabize(w: u32) -> u32 {w + 0x100000}

// Special composed key defintions. These values are added to 0x100000.
pub const NCKEY_INVALID :u32 = suppuabize(0);
pub const NCKEY_RESIZE  :u32 = suppuabize(1); // generated interally in response to SIGWINCH
pub const NCKEY_UP      :u32 = suppuabize(2);
pub const NCKEY_RIGHT   :u32 = suppuabize(3);
pub const NCKEY_DOWN    :u32 = suppuabize(4);
pub const NCKEY_LEFT    :u32 = suppuabize(5);
pub const NCKEY_INS     :u32 = suppuabize(6);
pub const NCKEY_DEL     :u32 = suppuabize(7);
pub const NCKEY_BACKSPACE :u32 = suppuabize(8); // backspace (sometimes)
pub const NCKEY_PGDOWN  :u32 = suppuabize(9);
pub const NCKEY_PGUP    :u32 = suppuabize(10);
pub const NCKEY_HOME    :u32 = suppuabize(11);
pub const NCKEY_END     :u32 = suppuabize(12);
pub const NCKEY_F00     :u32 = suppuabize(20);
pub const NCKEY_F01     :u32 = suppuabize(21);
pub const NCKEY_F02     :u32 = suppuabize(22);
pub const NCKEY_F03     :u32 = suppuabize(23);
pub const NCKEY_F04     :u32 = suppuabize(24);
pub const NCKEY_F05     :u32 = suppuabize(25);
pub const NCKEY_F06     :u32 = suppuabize(26);
pub const NCKEY_F07     :u32 = suppuabize(27);
pub const NCKEY_F08     :u32 = suppuabize(28);
pub const NCKEY_F09     :u32 = suppuabize(29);
pub const NCKEY_F10     :u32 = suppuabize(30);
pub const NCKEY_F11     :u32 = suppuabize(31);
pub const NCKEY_F12     :u32 = suppuabize(32);
pub const NCKEY_F13     :u32 = suppuabize(33);
pub const NCKEY_F14     :u32 = suppuabize(34);
pub const NCKEY_F15     :u32 = suppuabize(35);
pub const NCKEY_F16     :u32 = suppuabize(36);
pub const NCKEY_F17     :u32 = suppuabize(37);
pub const NCKEY_F18     :u32 = suppuabize(38);
pub const NCKEY_F19     :u32 = suppuabize(39);
pub const NCKEY_F20     :u32 = suppuabize(40);
pub const NCKEY_F21     :u32 = suppuabize(41);
pub const NCKEY_F22     :u32 = suppuabize(42);
pub const NCKEY_F23     :u32 = suppuabize(43);
pub const NCKEY_F24     :u32 = suppuabize(44);
pub const NCKEY_F25     :u32 = suppuabize(45);
pub const NCKEY_F26     :u32 = suppuabize(46);
pub const NCKEY_F27     :u32 = suppuabize(47);
pub const NCKEY_F28     :u32 = suppuabize(48);
pub const NCKEY_F29     :u32 = suppuabize(49);
pub const NCKEY_F30     :u32 = suppuabize(50);
pub const NCKEY_F31     :u32 = suppuabize(51);
pub const NCKEY_F32     :u32 = suppuabize(52);
pub const NCKEY_F33     :u32 = suppuabize(53);
pub const NCKEY_F34     :u32 = suppuabize(54);
pub const NCKEY_F35     :u32 = suppuabize(55);
pub const NCKEY_F36     :u32 = suppuabize(56);
pub const NCKEY_F37     :u32 = suppuabize(57);
pub const NCKEY_F38     :u32 = suppuabize(58);
pub const NCKEY_F39     :u32 = suppuabize(59);
pub const NCKEY_F40     :u32 = suppuabize(60);
pub const NCKEY_F41     :u32 = suppuabize(61);
pub const NCKEY_F42     :u32 = suppuabize(62);
pub const NCKEY_F43     :u32 = suppuabize(63);
pub const NCKEY_F44     :u32 = suppuabize(64);
pub const NCKEY_F45     :u32 = suppuabize(65);
pub const NCKEY_F46     :u32 = suppuabize(66);
pub const NCKEY_F47     :u32 = suppuabize(67);
pub const NCKEY_F48     :u32 = suppuabize(68);
pub const NCKEY_F49     :u32 = suppuabize(69);
pub const NCKEY_F50     :u32 = suppuabize(70);
pub const NCKEY_F51     :u32 = suppuabize(71);
pub const NCKEY_F52     :u32 = suppuabize(72);
pub const NCKEY_F53     :u32 = suppuabize(73);
pub const NCKEY_F54     :u32 = suppuabize(74);
pub const NCKEY_F55     :u32 = suppuabize(75);
pub const NCKEY_F56     :u32 = suppuabize(76);
pub const NCKEY_F57     :u32 = suppuabize(77);
pub const NCKEY_F58     :u32 = suppuabize(78);
pub const NCKEY_F59     :u32 = suppuabize(79);
pub const NCKEY_F60     :u32 = suppuabize(80);
// ... leave room for up to 100 function keys, egads
pub const NCKEY_ENTER   :u32 = suppuabize(121);
pub const NCKEY_CLS     :u32 = suppuabize(122); // "clear-screen or erase"
pub const NCKEY_DLEFT   :u32 = suppuabize(123); // down + left on keypad
pub const NCKEY_DRIGHT  :u32 = suppuabize(124);
pub const NCKEY_ULEFT   :u32 = suppuabize(125); // up + left on keypad
pub const NCKEY_URIGHT  :u32 = suppuabize(126);
pub const NCKEY_CENTER  :u32 = suppuabize(127); // the most truly neutral of keypresses
pub const NCKEY_BEGIN   :u32 = suppuabize(128);
pub const NCKEY_CANCEL  :u32 = suppuabize(129);
pub const NCKEY_CLOSE   :u32 = suppuabize(130);
pub const NCKEY_COMMAND :u32 = suppuabize(131);
pub const NCKEY_COPY    :u32 = suppuabize(132);
pub const NCKEY_EXIT    :u32 = suppuabize(133);
pub const NCKEY_PRINT   :u32 = suppuabize(134);
pub const NCKEY_REFRESH :u32 = suppuabize(135);
// Mouse events. We try to encode some details into the char32_t (i.e. which
// button was pressed);, but some is embedded in the ncinput event. The release
// event is generic across buttons; callers must maintain state, if they care.
pub const NCKEY_BUTTON1  :u32 = suppuabize(201);
pub const NCKEY_BUTTON2  :u32 = suppuabize(202);
pub const NCKEY_BUTTON3  :u32 = suppuabize(203);
pub const NCKEY_BUTTON4  :u32 = suppuabize(204); // scrollwheel up
pub const NCKEY_BUTTON5  :u32 = suppuabize(205); // scrollwheel down
pub const NCKEY_BUTTON6  :u32 = suppuabize(206);
pub const NCKEY_BUTTON7  :u32 = suppuabize(207);
pub const NCKEY_BUTTON8  :u32 = suppuabize(208);
pub const NCKEY_BUTTON9  :u32 = suppuabize(209);
pub const NCKEY_BUTTON10 :u32 = suppuabize(210);
pub const NCKEY_BUTTON11 :u32 = suppuabize(211);
pub const NCKEY_RELEASE  :u32 = suppuabize(212);

// Synonyms (so far as we're concerned)
pub const NCKEY_SCROLL_UP   :u32 = NCKEY_BUTTON4;
pub const NCKEY_SCROLL_DOWN :u32 = NCKEY_BUTTON5;
pub const NCKEY_RETURN      :u32 = NCKEY_ENTER;
