// #define CELL_INITIALIZER(c, s, chan) { .gcluster = (c), .gcluster_backstop = 0, .reserved = 0, .stylemask = (s), .channels = (chan), }
#[macro_export]
macro_rules! cell_initializer {
    ( $c:expr, $s:expr, $chan:expr  ) => {
        cell {
            gcluster: $c as u32,
            gcluster_backstop: 0 as EGCBackstop,
            reserved: 0,
            stylemask: $s,
            channels: $chan,
        }
    };
}

//#define CELL_SIMPLE_INITIALIZER(c) { .gcluster = (c), .gcluster_backstop = 0, .reserved = 0, .stylemask = 0, .channels = 0, }
#[macro_export]
macro_rules! cell_simple_initializer {
    ( $c:expr ) => {
        cell_initializer![$c, 0, 0]
    };
}

// #define CELL_TRIVIAL_INITIALIZER { }
#[macro_export]
macro_rules! cell_trivial_initializer {
    ( ) => {
        cell_simple_initializer![0]
    };
}
