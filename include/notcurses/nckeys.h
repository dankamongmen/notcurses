#ifndef NOTCURSES_NCKEYS
#define NOTCURSES_NCKEYS

#ifdef __cplusplus
extern "C" {
#endif

#define suppuabize(w) ((w) + 0x100000)

// Special composed key definitions. These values are added to 0x100000.
#define NCKEY_INVALID suppuabize(0)
#define NCKEY_SIGNAL  suppuabize(1) // we received either SIGWINCH or SIGCONT
#define NCKEY_UP      suppuabize(2)
#define NCKEY_RIGHT   suppuabize(3)
#define NCKEY_DOWN    suppuabize(4)
#define NCKEY_LEFT    suppuabize(5)
#define NCKEY_INS     suppuabize(6)
#define NCKEY_DEL     suppuabize(7)
#define NCKEY_BACKSPACE suppuabize(8) // backspace (sometimes)
#define NCKEY_PGDOWN  suppuabize(9)
#define NCKEY_PGUP    suppuabize(10)
#define NCKEY_HOME    suppuabize(11)
#define NCKEY_END     suppuabize(12)
#define NCKEY_F00     suppuabize(20)
#define NCKEY_F01     suppuabize(21)
#define NCKEY_F02     suppuabize(22)
#define NCKEY_F03     suppuabize(23)
#define NCKEY_F04     suppuabize(24)
#define NCKEY_F05     suppuabize(25)
#define NCKEY_F06     suppuabize(26)
#define NCKEY_F07     suppuabize(27)
#define NCKEY_F08     suppuabize(28)
#define NCKEY_F09     suppuabize(29)
#define NCKEY_F10     suppuabize(30)
#define NCKEY_F11     suppuabize(31)
#define NCKEY_F12     suppuabize(32)
#define NCKEY_F13     suppuabize(33)
#define NCKEY_F14     suppuabize(34)
#define NCKEY_F15     suppuabize(35)
#define NCKEY_F16     suppuabize(36)
#define NCKEY_F17     suppuabize(37)
#define NCKEY_F18     suppuabize(38)
#define NCKEY_F19     suppuabize(39)
#define NCKEY_F20     suppuabize(40)
#define NCKEY_F21     suppuabize(41)
#define NCKEY_F22     suppuabize(42)
#define NCKEY_F23     suppuabize(43)
#define NCKEY_F24     suppuabize(44)
#define NCKEY_F25     suppuabize(45)
#define NCKEY_F26     suppuabize(46)
#define NCKEY_F27     suppuabize(47)
#define NCKEY_F28     suppuabize(48)
#define NCKEY_F29     suppuabize(49)
#define NCKEY_F30     suppuabize(50)
#define NCKEY_F31     suppuabize(51)
#define NCKEY_F32     suppuabize(52)
#define NCKEY_F33     suppuabize(53)
#define NCKEY_F34     suppuabize(54)
#define NCKEY_F35     suppuabize(55)
#define NCKEY_F36     suppuabize(56)
#define NCKEY_F37     suppuabize(57)
#define NCKEY_F38     suppuabize(58)
#define NCKEY_F39     suppuabize(59)
#define NCKEY_F40     suppuabize(60)
#define NCKEY_F41     suppuabize(61)
#define NCKEY_F42     suppuabize(62)
#define NCKEY_F43     suppuabize(63)
#define NCKEY_F44     suppuabize(64)
#define NCKEY_F45     suppuabize(65)
#define NCKEY_F46     suppuabize(66)
#define NCKEY_F47     suppuabize(67)
#define NCKEY_F48     suppuabize(68)
#define NCKEY_F49     suppuabize(69)
#define NCKEY_F50     suppuabize(70)
#define NCKEY_F51     suppuabize(71)
#define NCKEY_F52     suppuabize(72)
#define NCKEY_F53     suppuabize(73)
#define NCKEY_F54     suppuabize(74)
#define NCKEY_F55     suppuabize(75)
#define NCKEY_F56     suppuabize(76)
#define NCKEY_F57     suppuabize(77)
#define NCKEY_F58     suppuabize(78)
#define NCKEY_F59     suppuabize(79)
#define NCKEY_F60     suppuabize(80)
// ... leave room for up to 100 function keys, egads
#define NCKEY_ENTER   suppuabize(121)
#define NCKEY_CLS     suppuabize(122) // "clear-screen or erase"
#define NCKEY_DLEFT   suppuabize(123) // down + left on keypad
#define NCKEY_DRIGHT  suppuabize(124)
#define NCKEY_ULEFT   suppuabize(125) // up + left on keypad
#define NCKEY_URIGHT  suppuabize(126)
#define NCKEY_CENTER  suppuabize(127) // the most truly neutral of keypresses
#define NCKEY_BEGIN   suppuabize(128)
#define NCKEY_CANCEL  suppuabize(129)
#define NCKEY_CLOSE   suppuabize(130)
#define NCKEY_COMMAND suppuabize(131)
#define NCKEY_COPY    suppuabize(132)
#define NCKEY_EXIT    suppuabize(133)
#define NCKEY_PRINT   suppuabize(134)
#define NCKEY_REFRESH suppuabize(135)
// Mouse events. We try to encode some details into the char32_t (i.e. which
// button was pressed), but some is embedded in the ncinput event. The release
// event is generic across buttons; callers must maintain state, if they care.
#define NCKEY_BUTTON1  suppuabize(201)
#define NCKEY_BUTTON2  suppuabize(202)
#define NCKEY_BUTTON3  suppuabize(203)
#define NCKEY_BUTTON4  suppuabize(204) // scrollwheel up
#define NCKEY_BUTTON5  suppuabize(205) // scrollwheel down
#define NCKEY_BUTTON6  suppuabize(206)
#define NCKEY_BUTTON7  suppuabize(207)
#define NCKEY_BUTTON8  suppuabize(208)
#define NCKEY_BUTTON9  suppuabize(209)
#define NCKEY_BUTTON10 suppuabize(210)
#define NCKEY_BUTTON11 suppuabize(211)
#define NCKEY_RELEASE  suppuabize(212)

// Synonyms (so far as we're concerned)
#define NCKEY_SCROLL_UP   NCKEY_BUTTON4
#define NCKEY_SCROLL_DOWN NCKEY_BUTTON5
#define NCKEY_RETURN      NCKEY_ENTER
#define NCKEY_RESIZE      NCKEY_SIGNAL

// Just aliases, ma'am, from the 128 characters common to ASCII+UTF8
#define NCKEY_ESC      0x1b
#define NCKEY_SPACE    0x20

#ifdef __cplusplus
} // extern "C"
#endif

#endif
