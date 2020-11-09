#[allow(unused_imports)]

use crate::Cell;

// Cell ------------------------------------------------------------------------

/// Initializes a cell providing an [`Egc`](type.Egc.html),
/// a [`StyleMask`](type.StyleMask.html) and [`Channels`](type.Channels.html)
#[macro_export]
macro_rules! cell_initializer {
    ( $c:expr, $s:expr, $chan:expr  ) => {
        Cell {
            gcluster: $c as u32,
            gcluster_backstop: 0 as EgcBackstop,
            reserved: 0,
            stylemask: $s,
            channels: $chan,
        }
    };
}

/// Initializes a cell providing just an [`Egc`](type.Egc.html),
#[macro_export]
macro_rules! cell_char_initializer {
    ( $c:expr ) => {
        cell_initializer![$c, 0, 0]
    };
}

/// Initializes an empty cell
#[macro_export]
macro_rules! cell_trivial_initializer {
    ( ) => {
        cell_char_initializer![0]
    };
}

// ncmetric --------------------------------------------------------------------

// /// Used as arguments to a variable field width (i.e. "%*s" -- these are the *).
// ///
// /// We need this convoluted grotesquery to properly handle 'µ'.
// //
// // TODO: TEST
// //
// // NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - ncstrwidth(x) + (cols)))
// #[macro_export]
// macro_rules! NCMETRICFWIDTH {
//     ( $x:expr, $cols:expr ) => {
//         libc::strlen($x) as i32 - ncstrwidth($x) as i32 + $cols as i32
//     }
// }
//
// Proof of concept as const fn (wont work because strlen call)
// pub const unsafe fn NCMETRICFWIDTH(x: i8, cols: usize) -> usize {
//     libc::strlen(&x) - crate::ncstrwidth(&x) as usize + cols
// }

// #define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - ncstrwidth(x) + (cols)))
// #define PREFIXFMT(x) NCMETRICFWIDTH((x), PREFIXCOLUMNS), (x)
// #define IPREFIXFMT(x) NCMETRIXFWIDTH((x), IPREFIXCOLUMNS), (x)
// #define BPREFIXFMT(x) NCMETRICFWIDTH((x), BPREFIXCOLUMNS), (x)

