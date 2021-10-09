#ifndef NOTCURSES_NCKEYS
#define NOTCURSES_NCKEYS

#ifdef __cplusplus
extern "C" {
#endif

// we place our synthesized codepoints beyond the reach of Unicode's 17
// possible planes (limit due to UTF16). this way, none of our synthesized
// keys ought map to a real unicode character.
#define beyondunicode(w) ((w) + 0x110000)

#define NCKEY_INVALID   beyondunicode(0)
#define NCKEY_SIGNAL    beyondunicode(1) // we received either SIGWINCH or SIGCONT
#define NCKEY_UP        beyondunicode(2)
#define NCKEY_RIGHT     beyondunicode(3)
#define NCKEY_DOWN      beyondunicode(4)
#define NCKEY_LEFT      beyondunicode(5)
#define NCKEY_INS       beyondunicode(6)
#define NCKEY_DEL       beyondunicode(7)
#define NCKEY_BACKSPACE beyondunicode(8) // backspace (sometimes)
#define NCKEY_PGDOWN    beyondunicode(9)
#define NCKEY_PGUP      beyondunicode(10)
#define NCKEY_HOME      beyondunicode(11)
#define NCKEY_END       beyondunicode(12)
#define NCKEY_F00       beyondunicode(20)
#define NCKEY_F01       beyondunicode(21)
#define NCKEY_F02       beyondunicode(22)
#define NCKEY_F03       beyondunicode(23)
#define NCKEY_F04       beyondunicode(24)
#define NCKEY_F05       beyondunicode(25)
#define NCKEY_F06       beyondunicode(26)
#define NCKEY_F07       beyondunicode(27)
#define NCKEY_F08       beyondunicode(28)
#define NCKEY_F09       beyondunicode(29)
#define NCKEY_F10       beyondunicode(30)
#define NCKEY_F11       beyondunicode(31)
#define NCKEY_F12       beyondunicode(32)
#define NCKEY_F13       beyondunicode(33)
#define NCKEY_F14       beyondunicode(34)
#define NCKEY_F15       beyondunicode(35)
#define NCKEY_F16       beyondunicode(36)
#define NCKEY_F17       beyondunicode(37)
#define NCKEY_F18       beyondunicode(38)
#define NCKEY_F19       beyondunicode(39)
#define NCKEY_F20       beyondunicode(40)
#define NCKEY_F21       beyondunicode(41)
#define NCKEY_F22       beyondunicode(42)
#define NCKEY_F23       beyondunicode(43)
#define NCKEY_F24       beyondunicode(44)
#define NCKEY_F25       beyondunicode(45)
#define NCKEY_F26       beyondunicode(46)
#define NCKEY_F27       beyondunicode(47)
#define NCKEY_F28       beyondunicode(48)
#define NCKEY_F29       beyondunicode(49)
#define NCKEY_F30       beyondunicode(50)
#define NCKEY_F31       beyondunicode(51)
#define NCKEY_F32       beyondunicode(52)
#define NCKEY_F33       beyondunicode(53)
#define NCKEY_F34       beyondunicode(54)
#define NCKEY_F35       beyondunicode(55)
#define NCKEY_F36       beyondunicode(56)
#define NCKEY_F37       beyondunicode(57)
#define NCKEY_F38       beyondunicode(58)
#define NCKEY_F39       beyondunicode(59)
#define NCKEY_F40       beyondunicode(60)
#define NCKEY_F41       beyondunicode(61)
#define NCKEY_F42       beyondunicode(62)
#define NCKEY_F43       beyondunicode(63)
#define NCKEY_F44       beyondunicode(64)
#define NCKEY_F45       beyondunicode(65)
#define NCKEY_F46       beyondunicode(66)
#define NCKEY_F47       beyondunicode(67)
#define NCKEY_F48       beyondunicode(68)
#define NCKEY_F49       beyondunicode(69)
#define NCKEY_F50       beyondunicode(70)
#define NCKEY_F51       beyondunicode(71)
#define NCKEY_F52       beyondunicode(72)
#define NCKEY_F53       beyondunicode(73)
#define NCKEY_F54       beyondunicode(74)
#define NCKEY_F55       beyondunicode(75)
#define NCKEY_F56       beyondunicode(76)
#define NCKEY_F57       beyondunicode(77)
#define NCKEY_F58       beyondunicode(78)
#define NCKEY_F59       beyondunicode(79)
#define NCKEY_F60       beyondunicode(80)
// ... leave room for up to 100 function keys, egads
#define NCKEY_ENTER     beyondunicode(121)
#define NCKEY_CLS       beyondunicode(122) // "clear-screen or erase"
#define NCKEY_DLEFT     beyondunicode(123) // down + left on keypad
#define NCKEY_DRIGHT    beyondunicode(124)
#define NCKEY_ULEFT     beyondunicode(125) // up + left on keypad
#define NCKEY_URIGHT    beyondunicode(126)
#define NCKEY_CENTER    beyondunicode(127) // the most truly neutral of keypresses
#define NCKEY_BEGIN     beyondunicode(128)
#define NCKEY_CANCEL    beyondunicode(129)
#define NCKEY_CLOSE     beyondunicode(130)
#define NCKEY_COMMAND   beyondunicode(131)
#define NCKEY_COPY      beyondunicode(132)
#define NCKEY_EXIT      beyondunicode(133)
#define NCKEY_PRINT     beyondunicode(134)
#define NCKEY_REFRESH   beyondunicode(135)
// these keys aren't generally available outside of the kitty protocol
#define NCKEY_CAPS_LOCK    beyondunicode(150)
#define NCKEY_SCROLL_LOCK  beyondunicode(151)
#define NCKEY_NUM_LOCK     beyondunicode(152)
#define NCKEY_PRINT_SCREEN beyondunicode(153)
#define NCKEY_PAUSE        beyondunicode(154)
#define NCKEY_MENU         beyondunicode(155)
// media keys, similarly only available through kitty's protocol
#define NCKEY_MEDIA_PLAY   beyondunicode(158)
#define NCKEY_MEDIA_PAUSE  beyondunicode(159)
#define NCKEY_MEDIA_PPAUSE beyondunicode(160)
#define NCKEY_MEDIA_REV    beyondunicode(161)
#define NCKEY_MEDIA_STOP   beyondunicode(162)
#define NCKEY_MEDIA_FF     beyondunicode(163)
#define NCKEY_MEDIA_REWIND beyondunicode(164)
#define NCKEY_MEDIA_NEXT   beyondunicode(165)
#define NCKEY_MEDIA_PREV   beyondunicode(166)
#define NCKEY_MEDIA_RECORD beyondunicode(167)
#define NCKEY_MEDIA_LVOL   beyondunicode(168)
#define NCKEY_MEDIA_RVOL   beyondunicode(169)
#define NCKEY_MEDIA_MUTE   beyondunicode(170)
// modifiers when pressed by themselves. this ordering comes from the Kitty
// keyboard protocol, and mustn't be changed without updating handlers.
#define NCKEY_LSHIFT       beyondunicode(171)
#define NCKEY_LCTRL        beyondunicode(172)
#define NCKEY_LALT         beyondunicode(173)
#define NCKEY_LSUPER       beyondunicode(174)
#define NCKEY_LHYPER       beyondunicode(175)
#define NCKEY_LMETA        beyondunicode(176)
#define NCKEY_RSHIFT       beyondunicode(177)
#define NCKEY_RCTRL        beyondunicode(178)
#define NCKEY_RALT         beyondunicode(179)
#define NCKEY_RSUPER       beyondunicode(180)
#define NCKEY_RHYPER       beyondunicode(181)
#define NCKEY_RMETA        beyondunicode(182)
// mouse events. We try to encode some details into the char32_t (i.e. which
// button was pressed), but some is embedded in the ncinput event. the release
// event is generic across buttons; callers must maintain state, if they care.
#define NCKEY_BUTTON1   beyondunicode(201)
#define NCKEY_BUTTON2   beyondunicode(202)
#define NCKEY_BUTTON3   beyondunicode(203)
#define NCKEY_BUTTON4   beyondunicode(204) // scrollwheel up
#define NCKEY_BUTTON5   beyondunicode(205) // scrollwheel down
#define NCKEY_BUTTON6   beyondunicode(206)
#define NCKEY_BUTTON7   beyondunicode(207)
#define NCKEY_BUTTON8   beyondunicode(208)
#define NCKEY_BUTTON9   beyondunicode(209)
#define NCKEY_BUTTON10  beyondunicode(210)
#define NCKEY_BUTTON11  beyondunicode(211)

// indicates that we have reached the end of input. any further calls
// will continute to return this immediately.
#define NCKEY_EOF       beyondunicode(300)

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
