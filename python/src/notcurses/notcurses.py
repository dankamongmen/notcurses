import sys
import locale
from _notcurses import lib, ffi

class Notcurses:
    def __init__(self):
        opts = ffi.new("notcurses_options *")
        self.nc = lib.notcurses_init(opts, sys.stdout)

    def __del__(self):
        lib.notcurses_stop(self.nc)

    def render(self):
        return lib.notcurses_render(self.nc)

if __name__ == '__main__':
    locale.setlocale(locale.LC_ALL, "")
    nc = Notcurses()
    nc.render()
    print('ohhhh yeah')
