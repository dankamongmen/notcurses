//! `NcSigSet`

use crate::NcIntResult;

/// A wrapper over
/// [sigset_t](https://www.gnu.org/software/libc/manual/html_node/Signal-Sets.html).
//
// Expected by [`notcurses_getc`], [`notcurses_getc_nblock`],
// [`ncdirect_getc`] & [`ncdirect_getc_nblock`], that can't use libc::sigset_t
pub type NcSignalSet = crate::bindings::ffi::sigset_t;

impl NcSignalSet {
    /// New NcSignalSet.
    pub fn new() -> Self {
        // https://github.com/dankamongmen/notcurses/issues/1339
        #[cfg(any(target_arch = "armv7l", target_arch = "i686"))]
        return Self { __val: [0; 32] };
        #[cfg(not(any(target_arch = "armv7l", target_arch = "i686")))]
        return Self { __val: [0; 16] };
    }

    /// Adds `signum` to this set.
    pub fn addset(&mut self, signum: i32) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigaddset(self, signum) }
    }

    /// Removes `signum` from this set.
    pub fn delset(&mut self, signum: i32) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigdelset(self, signum) }
    }

    /// Clears all signals from this set.
    pub fn emptyset(&mut self) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigemptyset(self) }
    }

    /// Sets all signals in this set.
    pub fn fillset(&mut self) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigfillset(self) }
    }

    /// Is `signum` a member of this set?
    pub fn ismember(&self, signum: i32) -> bool {
        if unsafe { crate::bindings::ffi::sigismember(self, signum) } == 1 {
            return true;
        }
        false
    }

    /// Puts in this set all signals that are blocked and waiting to be delivered.
    pub fn pending(&mut self) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigpending(self) }
    }

    /// Gets and/or changes the set of blocked signals.
    //
    // https://linux.die.net/man/2/sigprocmask
    pub fn procmask(how: i32, set: &NcSignalSet, old_set: &mut NcSignalSet) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigprocmask(how, set, old_set) }
    }

    /// Changes the set of blocked signals to the ones in this set,
    /// waits until a signal arrives, and restores the set of blocked signals.
    pub fn suspend(&self) -> NcIntResult {
        unsafe { crate::bindings::ffi::sigsuspend(self) }
    }
}
