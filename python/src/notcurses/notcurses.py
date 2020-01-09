import sys
import locale
from _notcurses import lib, ffi

class Cell:
    def __init__(self, ncplane):
        self.c = ffi.new("cell *")
        self.c.gcluster = 0x20
        self.c.channels = 0
        self.c.attrword = 0
        self.ncp = ncplane

    def __del__(self):
        lib.cell_release(self.ncp.getNcplane(), self.c)

    def setBgRGB(self, r, g, b):
        if r < 0 or r > 255:
            raise ValueError("Bad red value")
        if g < 0 or g > 255:
            raise ValueError("Bad green value")
        if b < 0 or b > 255:
            raise ValueError("Bad blue value")
        self.c.channels = r * 65536 + g * 256 + b # FIXME

    def getNccell(self):
        return self.c

class Ncplane:
    def __init__(self, plane):
        self.n = plane

    def setBase(self, cell):
        return lib.ncplane_set_base(self.n, cell.getNccell())

    def getNcplane(self):
        return self.n

    def putSimpleYX(self, y, x, ch):
        return lib.ncplane_putsimple_yx(self.n, y, x, ch)

    def getDimensions(self):
        y = ffi.new("int *")
        x = ffi.new("int *")
        lib.ncplane_dim_yx(self.n, y, x)
        return (y[0], x[0])

class Notcurses:
    def __init__(self):
        opts = ffi.new("notcurses_options *")
        opts.inhibit_alternate_screen = True
        self.nc = lib.notcurses_init(opts, sys.stdout)
        self.stdncplane = Ncplane(lib.notcurses_stdplane(self.nc))

    def __del__(self):
        lib.notcurses_stop(self.nc)

    def render(self):
        return lib.notcurses_render(self.nc)

    def stdplane(self):
        return self.stdncplane

if __name__ == '__main__':
    locale.setlocale(locale.LC_ALL, "")
    nc = Notcurses()
    c = Cell(nc.stdplane())
    c.setBgRGB(0x80, 0xc0, 0x80)
    nc.stdplane().setBase(c)
    dims = nc.stdplane().getDimensions()
    for y in range(dims[0]):
        for x in range(dims[1]):
            nc.stdplane().putSimpleYX(y, x, b'X')
    nc.render()
