#[allow(unused_imports)]

use crate::Cell;

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

#[macro_export]
macro_rules! cell_simple_initializer {
    ( $c:expr ) => {
        cell_initializer![$c, 0, 0]
    };
}

#[macro_export]
macro_rules! cell_trivial_initializer {
    ( ) => {
        cell_simple_initializer![0]
    };
}
