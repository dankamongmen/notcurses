//! Special composed key definitions. These values are added to 0x100000.

use std::mem::transmute;

// NOTE: These defined macros can't be handled by bindgen yet, see:
//  - https://github.com/rust-lang/rust-bindgen/issues/316
//  - https://github.com/jethrogb/rust-cexpr/pull/15

// Waiting for: https://github.com/rust-lang/rust/issues/53605
//const fn suppuabize(w: u32) -> char {
const fn suppuabize(w: u32) -> u32 {
    // unsafe { transmute(w + 0x100000) }
    w + 0x100000
}

pub const NCKEY_INVALID: char = unsafe { transmute(suppuabize(0)) };
///generated interally in response to SIGWINCH
pub const NCKEY_RESIZE: char = unsafe { transmute(suppuabize(1)) };
pub const NCKEY_UP: char = unsafe { transmute(suppuabize(2)) };
pub const NCKEY_RIGHT: char = unsafe { transmute(suppuabize(3)) };
pub const NCKEY_DOWN: char = unsafe { transmute(suppuabize(4)) };
pub const NCKEY_LEFT: char = unsafe { transmute(suppuabize(5)) };
pub const NCKEY_INS: char = unsafe { transmute(suppuabize(6)) };
pub const NCKEY_DEL: char = unsafe { transmute(suppuabize(7)) };
/// backspace (sometimes)
pub const NCKEY_BACKSPACE: char = unsafe { transmute(suppuabize(8)) };
pub const NCKEY_PGDOWN: char = unsafe { transmute(suppuabize(9)) };
pub const NCKEY_PGUP: char = unsafe { transmute(suppuabize(10)) };
pub const NCKEY_HOME: char = unsafe { transmute(suppuabize(11)) };
pub const NCKEY_END: char = unsafe { transmute(suppuabize(12)) };
pub const NCKEY_F00: char = unsafe { transmute(suppuabize(20)) };
pub const NCKEY_F01: char = unsafe { transmute(suppuabize(21)) };
pub const NCKEY_F02: char = unsafe { transmute(suppuabize(22)) };
pub const NCKEY_F03: char = unsafe { transmute(suppuabize(23)) };
pub const NCKEY_F04: char = unsafe { transmute(suppuabize(24)) };
pub const NCKEY_F05: char = unsafe { transmute(suppuabize(25)) };
pub const NCKEY_F06: char = unsafe { transmute(suppuabize(26)) };
pub const NCKEY_F07: char = unsafe { transmute(suppuabize(27)) };
pub const NCKEY_F08: char = unsafe { transmute(suppuabize(28)) };
pub const NCKEY_F09: char = unsafe { transmute(suppuabize(29)) };
pub const NCKEY_F10: char = unsafe { transmute(suppuabize(30)) };
pub const NCKEY_F11: char = unsafe { transmute(suppuabize(31)) };
pub const NCKEY_F12: char = unsafe { transmute(suppuabize(32)) };
pub const NCKEY_F13: char = unsafe { transmute(suppuabize(33)) };
pub const NCKEY_F14: char = unsafe { transmute(suppuabize(34)) };
pub const NCKEY_F15: char = unsafe { transmute(suppuabize(35)) };
pub const NCKEY_F16: char = unsafe { transmute(suppuabize(36)) };
pub const NCKEY_F17: char = unsafe { transmute(suppuabize(37)) };
pub const NCKEY_F18: char = unsafe { transmute(suppuabize(38)) };
pub const NCKEY_F19: char = unsafe { transmute(suppuabize(39)) };
pub const NCKEY_F20: char = unsafe { transmute(suppuabize(40)) };
pub const NCKEY_F21: char = unsafe { transmute(suppuabize(41)) };
pub const NCKEY_F22: char = unsafe { transmute(suppuabize(42)) };
pub const NCKEY_F23: char = unsafe { transmute(suppuabize(43)) };
pub const NCKEY_F24: char = unsafe { transmute(suppuabize(44)) };
pub const NCKEY_F25: char = unsafe { transmute(suppuabize(45)) };
pub const NCKEY_F26: char = unsafe { transmute(suppuabize(46)) };
pub const NCKEY_F27: char = unsafe { transmute(suppuabize(47)) };
pub const NCKEY_F28: char = unsafe { transmute(suppuabize(48)) };
pub const NCKEY_F29: char = unsafe { transmute(suppuabize(49)) };
pub const NCKEY_F30: char = unsafe { transmute(suppuabize(50)) };
pub const NCKEY_F31: char = unsafe { transmute(suppuabize(51)) };
pub const NCKEY_F32: char = unsafe { transmute(suppuabize(52)) };
pub const NCKEY_F33: char = unsafe { transmute(suppuabize(53)) };
pub const NCKEY_F34: char = unsafe { transmute(suppuabize(54)) };
pub const NCKEY_F35: char = unsafe { transmute(suppuabize(55)) };
pub const NCKEY_F36: char = unsafe { transmute(suppuabize(56)) };
pub const NCKEY_F37: char = unsafe { transmute(suppuabize(57)) };
pub const NCKEY_F38: char = unsafe { transmute(suppuabize(58)) };
pub const NCKEY_F39: char = unsafe { transmute(suppuabize(59)) };
pub const NCKEY_F40: char = unsafe { transmute(suppuabize(60)) };
pub const NCKEY_F41: char = unsafe { transmute(suppuabize(61)) };
pub const NCKEY_F42: char = unsafe { transmute(suppuabize(62)) };
pub const NCKEY_F43: char = unsafe { transmute(suppuabize(63)) };
pub const NCKEY_F44: char = unsafe { transmute(suppuabize(64)) };
pub const NCKEY_F45: char = unsafe { transmute(suppuabize(65)) };
pub const NCKEY_F46: char = unsafe { transmute(suppuabize(66)) };
pub const NCKEY_F47: char = unsafe { transmute(suppuabize(67)) };
pub const NCKEY_F48: char = unsafe { transmute(suppuabize(68)) };
pub const NCKEY_F49: char = unsafe { transmute(suppuabize(69)) };
pub const NCKEY_F50: char = unsafe { transmute(suppuabize(70)) };
pub const NCKEY_F51: char = unsafe { transmute(suppuabize(71)) };
pub const NCKEY_F52: char = unsafe { transmute(suppuabize(72)) };
pub const NCKEY_F53: char = unsafe { transmute(suppuabize(73)) };
pub const NCKEY_F54: char = unsafe { transmute(suppuabize(74)) };
pub const NCKEY_F55: char = unsafe { transmute(suppuabize(75)) };
pub const NCKEY_F56: char = unsafe { transmute(suppuabize(76)) };
pub const NCKEY_F57: char = unsafe { transmute(suppuabize(77)) };
pub const NCKEY_F58: char = unsafe { transmute(suppuabize(78)) };
pub const NCKEY_F59: char = unsafe { transmute(suppuabize(79)) };
pub const NCKEY_F60: char = unsafe { transmute(suppuabize(80)) };

// ... leave room for up to 100 function keys, egads

pub const NCKEY_ENTER: char = unsafe { transmute(suppuabize(121)) };
/// "clear-screen or erase"
pub const NCKEY_CLS: char = unsafe { transmute(suppuabize(122)) };
/// down + left on keypad
pub const NCKEY_DLEFT: char = unsafe { transmute(suppuabize(123)) };
pub const NCKEY_DRIGHT: char = unsafe { transmute(suppuabize(124)) };
/// up + left on keypad
pub const NCKEY_ULEFT: char = unsafe { transmute(suppuabize(125)) };
pub const NCKEY_URIGHT: char = unsafe { transmute(suppuabize(126)) };
/// the most truly neutral of keypresses
pub const NCKEY_CENTER: char = unsafe { transmute(suppuabize(127)) };
pub const NCKEY_BEGIN: char = unsafe { transmute(suppuabize(128)) };
pub const NCKEY_CANCEL: char = unsafe { transmute(suppuabize(129)) };
pub const NCKEY_CLOSE: char = unsafe { transmute(suppuabize(130)) };
pub const NCKEY_COMMAND: char = unsafe { transmute(suppuabize(131)) };
pub const NCKEY_COPY: char = unsafe { transmute(suppuabize(132)) };
pub const NCKEY_EXIT: char = unsafe { transmute(suppuabize(133)) };
pub const NCKEY_PRINT: char = unsafe { transmute(suppuabize(134)) };
pub const NCKEY_REFRESH: char = unsafe { transmute(suppuabize(135)) };

// Mouse events. We try to encode some details into the char32_t (i.e. which
// button was pressed);, but some is embedded in the ncinput event. The release
// event is generic across buttons; callers must maintain state, if they care.
pub const NCKEY_BUTTON1: char = unsafe { transmute(suppuabize(201)) };
pub const NCKEY_BUTTON2: char = unsafe { transmute(suppuabize(202)) };
pub const NCKEY_BUTTON3: char = unsafe { transmute(suppuabize(203)) };
/// scrollwheel up
pub const NCKEY_BUTTON4: char = unsafe { transmute(suppuabize(204)) };
/// scrollwheel down
pub const NCKEY_BUTTON5: char = unsafe { transmute(suppuabize(205)) };
pub const NCKEY_BUTTON6: char = unsafe { transmute(suppuabize(206)) };
pub const NCKEY_BUTTON7: char = unsafe { transmute(suppuabize(207)) };
pub const NCKEY_BUTTON8: char = unsafe { transmute(suppuabize(208)) };
pub const NCKEY_BUTTON9: char = unsafe { transmute(suppuabize(209)) };
pub const NCKEY_BUTTON10: char = unsafe { transmute(suppuabize(210)) };
pub const NCKEY_BUTTON11: char = unsafe { transmute(suppuabize(211)) };
pub const NCKEY_RELEASE: char = unsafe { transmute(suppuabize(212)) };

// Synonyms (so far as we're concerned)
pub const NCKEY_SCROLL_UP: char = NCKEY_BUTTON4;
pub const NCKEY_SCROLL_DOWN: char = NCKEY_BUTTON5;
pub const NCKEY_RETURN: char = NCKEY_ENTER;
