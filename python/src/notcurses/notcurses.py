#!/usr/bin/python3

import sys
import locale
import _cffi_backend
from _notcurses import lib, ffi

def checkRGB(r, g, b):
    if r < 0 or r > 255:
        raise ValueError("Bad red value")
    if g < 0 or g > 255:
        raise ValueError("Bad green value")
    if b < 0 or b > 255:
        raise ValueError("Bad blue value")

class Cell:
    def __init__(self, ncplane):
        self.c = ffi.new("cell *")
        self.c.gcluster = 0x20
        self.c.channels = 0
        self.c.attrword = 0
        self.ncp = ncplane

    def __del__(self):
        lib.cell_release(self.ncp.getNcplane(), self.c)

    def setFgRGB(self, r, g, b):
        checkRGB(r, g, b)
        lib.cell_set_fg_rgb(self.c, r, g, b)

    def setBgRGB(self, r, g, b):
        checkRGB(r, g, b)
        lib.cell_set_bg_rgb(self.c, r, g, b)

    def getNccell(self):
        return self.c

class Ncplane:
    def __init__(self, plane):
        self.n = plane

    def setBaseCell(self, cell):
        return lib.ncplane_set_base_cell(self.n, cell.getNccell())

    def getNcplane(self):
        return self.n

    def putSimpleYX(self, y, x, ch):
        return lib.ncplane_putsimple_yx(self.n, y, x, ch)

    def getDimensions(self):
        y = ffi.new("int *")
        x = ffi.new("int *")
        lib.ncplane_dim_yx(self.n, y, x)
        return (y[0], x[0])

    def setFgRGB(self, r, g, b):
        checkRGB(r, g, b)
        lib.ncplane_set_fg_rgb(self.n, r, g, b)

    def setBgRGB(self, r, g, b):
        checkRGB(r, g, b)
        lib.ncplane_set_bg_rgb(self.n, r, g, b)

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

class Ncdirect:
    def __init__(self):
        self.nc = lib.notcurses_directmode(ffi.NULL, sys.stdout)

    def __del__(self):
        lib.ncdirect_stop(self.nc)

    # FIXME ought be checking for errors on the actual library calls, also
    def setFgRGB8(self, r, g, b):
        checkRGB8(r, g, b)
        lib.ncdirect_fg_rgb8(self.nc, r, g, b)

    def setBgRGB8(self, r, g, b):
        checkRGB8(r, g, b)
        lib.ncdirect_bg_rgb8(self.nc, r, g, b)

    def setFg(self, rgb):
        checkRGB(rgb)
        lib.ncdirect_fg(self.nc, rgb)

    def setBg(self, rgb):
        checkRGB(rgb)
        lib.ncdirect_bg(self.nc, rgb)

if __name__ == '__main__':
    locale.setlocale(locale.LC_ALL, "")
    nc = Notcurses()
    c = Cell(nc.stdplane())
    c.setBgRGB(0x80, 0xc0, 0x80)
    nc.stdplane().setBaseCell(c)
    dims = nc.stdplane().getDimensions()
    r = 0x80
    g = 0x80
    b = 0x80
    for y in range(dims[0]):
        for x in range(dims[1]):
            nc.stdplane().setFgRGB(r, g, b)
            nc.stdplane().setBgRGB(b, r, g)
            nc.stdplane().putSimpleYX(y, x, b'X')
            b = b + 2
            if b == 256:
                b = 0
                g = g + 2
                if g == 256:
                    g = 0
                    r = r + 2
    nc.render()
