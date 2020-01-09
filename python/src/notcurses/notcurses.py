import sys
import locale
from _notcurses import lib, ffi

class Ncplane:
    def __init__(self, plane):
        self.n = plane

class Notcurses:
    def __init__(self):
        opts = ffi.new("notcurses_options *")
        #opts.inhibit_alternate_screen = True
        self.nc = lib.notcurses_init(opts, sys.stdout)
        self.stdplane = lib.notcurses_stdplane(self.nc)

    def __del__(self):
        lib.notcurses_stop(self.nc)

    def render(self):
        return lib.notcurses_render(self.nc)

    def stdplane(self):
        return self.stdplane

if __name__ == '__main__':
    locale.setlocale(locale.LC_ALL, "")
    nc = Notcurses()
    nc.render()
