from _notcurses import lib, ffi

class Notcurses:
    def __init__(self):
        self.nc = lib.notcurses_init()

    def __del__(self):
        lib.notcurses_stop(self.nc)

    def render():
        return lib.notcurses_render(self.nc)

def main():
    print('ohhhh yeah')
    nc = Notcurses()
    nc.render()
    print('ohhhh yeah')
