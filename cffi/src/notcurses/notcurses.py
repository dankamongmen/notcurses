#!/usr/bin/python3

import sys
import locale
import _cffi_backend
from _notcurses import lib, ffi

def channel_r(channel):
    return (channel & 0xff0000) >> 16;

def channel_g(channel):
    return (channel & 0x00ff00) >> 8;

def channel_b(channel):
    return (channel & 0x0000ff);

def channel_rgb8(channel):
    return (channel_r(channel), channel_g(channel), channel_b(channel))

def channel_set_rgb8(channel, r, g, b):
    checkRGB(r, g, b)
    c = (r << 16) | (g << 8) | b
    return (channel & ~lib.CELL_BG_RGB_MASK) | lib.CELL_BGDEFAULT_MASK | c

def channels_fchannel(channels):
    return channels & 0xffffffff00000000

def channels_bchannel(channels):
    return channels & 0xffffffff

def channels_fg_rgb8(channels):
    return channel_rgb8(channels_fchannel(channels))

def channels_set_fchannel(channels, channel):
    return (channel << 32) | (channels & 0xffffffff)

def channels_set_fg_rgb8(channels, r, g, b):
    channel = channels_fchannel(channels)
    channel = channel_set_rgb8(channel, r, g, b)
    return channels_set_fchannel(channels, channel)

def channels_bg_rgb8(channels):
    return channel_rgb8(channels_bchannel(channels))

def channels_set_bchannel(channels, channel):
    return (channels & 0xffffffff00000000) | channel;

def channels_set_bg_rgb8(channels, r, g, b):
    checkRGB(r, g, b)
    channel = channels_bchannel(channels)
    channel = channel_set_rgb8(channel, r, g, b)
    return channels_set_bchannel(channels, channel);

class NotcursesError(Exception):
    """Base class for notcurses exceptions."""
    def __init__(self, message):
        self.message = message

def checkRGB(r, g, b):
    if r < 0 or r > 255:
        raise ValueError("Bad red value")
    if g < 0 or g > 255:
        raise ValueError("Bad green value")
    if b < 0 or b > 255:
        raise ValueError("Bad blue value")

class Cell:
    def __init__(self, ncplane, egc):
        self.ncp = ncplane
        self.c = ffi.new("cell *")
        self.c.gcluster = egc # FIXME need use cell_load

    def __del__(self):
        lib.nccell_release(self.ncp.getNcplane(), self.c)

    def setFgRGB(self, r, g, b):
        self.c.channels = channels_set_fg_rgb8(self.c.channels, r, g, b)

    def setBgRGB(self, r, g, b):
        self.c.channels = channels_set_bg_rgb8(self.c.channels, r, g, b)

    def getNccell(self):
        return self.c

class Ncplane:
    def __init__(self, plane):
        self.n = plane

    def setBaseCell(self, cell):
        return lib.ncplane_set_base_cell(self.n, cell.getNccell())

    def getNcplane(self):
        return self.n

    def putEGCYX(self, y, x, egc):
        if y < -1:
            raise ValueError("Bad y position")
        if x < -1:
            raise ValueError("Bad x position")
        cstr = ffi.new("char[]", egc.encode('utf-8'))
        ret = lib.ncplane_putegc_yx(self.n, y, x, cstr, ffi.NULL)
        ffi.release(cstr)
        return ret

    def getDimensions(self):
        y = ffi.new("int *")
        x = ffi.new("int *")
        lib.ncplane_dim_yx(self.n, y, x)
        return (y[0], x[0])

    def getFChannel(self):
        return channels_fchannel(lib.ncplane_channels(self.n));

    def getBChannel(self):
        return channels_bchannel(lib.ncplane_channels(self.n));

    def getFgRGB(self):
        return channel_rgb8(self.getFChannel())

    def getBgRGB(self):
        return channel_rgb8(self.getBChannel())

    def setFgRGB(self, r, g, b):
        if lib.ncplane_set_fg_rgb8(self.n, r, g, b):
            raise ValueError("Bad foreground RGB")

    def setBgRGB(self, r, g, b):
        if lib.ncplane_set_bg_rgb8(self.n, r, g, b):
            raise ValueError("Bad background RGB")

class Notcurses:
    def __init__(self):
        opts = ffi.new("notcurses_options *")
        opts.flags = lib.NCOPTION_NO_ALTERNATE_SCREEN
        self.nc = lib.notcurses_init(opts, sys.stdout)
        if not self.nc:
            raise NotcursesError("Error initializing notcurses")
        self.stdncplane = Ncplane(lib.notcurses_stdplane(self.nc))

    def __del__(self):
        lib.notcurses_stop(self.nc)

    def render(self):
        return lib.notcurses_render(self.nc)

    def stdplane(self):
        return self.stdncplane

class Ncdirect:
    def __init__(self):
        self.nc = lib.ncdirect_init(ffi.NULL, sys.stdout, 0)

    def __del__(self):
        lib.ncdirect_stop(self.nc)

    def setFgRGB8(self, r, g, b):
        if lib.ncdirect_set_fg_rgb8(self.nc, r, g, b):
            raise ValueError("Bad foreground RGB")

    def setBgRGB8(self, r, g, b):
        if lib.ncdirect_set_bg_rgb8(self.nc, r, g, b):
            raise ValueError("Bad background RGB")

    def setFg(self, rgb):
        if lib.ncdirect_set_fg_rgb(self.nc, rgb):
            raise ValueError("Bad foreground RGB")

    def setBg(self, rgb):
        if lib.ncdirect_set_bg_rgb(self.nc, rgb):
            raise ValueError("Bad background RGB")

    def cursorEnable(self):
        if lib.ncdirect_cursor_enable(self.nc):
            raise RuntimeError("Couldn't enable terminal cursor");

    def cursorDisable(self):
        if lib.ncdirect_cursor_disable(self.nc):
            raise RuntimeError("Couldn't disable terminal cursor");

if __name__ == '__main__':
    locale.setlocale(locale.LC_ALL, "")
    nc = Notcurses()
    n = nc.stdplane()
    nc.render()
