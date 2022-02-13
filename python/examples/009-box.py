from time import sleep
import notcurses as nc

notcurses = nc.Notcurses()
plane = notcurses.stdplane()

BOX_CHARS = (
    nc.NCBOXASCII,
    nc.NCBOXDOUBLE,
    nc.NCBOXHEAVY,
    nc.NCBOXLIGHT,
    nc.NCBOXOUTER,
    nc.NCBOXROUND,
)

CHANNELS = (
    0,
    0x0000000040808080,  # default on grey
    0x40ff000000000000,  # red on default
    0x4000ff00400000ff,  # green on blue
)

SY = 7
SX = 10

for y, channels in enumerate(CHANNELS):
    for x, box_chars in enumerate(BOX_CHARS):
        nc.box(
            plane, (y + 1) * SY - 1, (x + 1) * SX - 1, y * SY + 1, x * SX + 1,
            box_chars,
            channels=channels,
            # ctlword=0x1f9
        )

notcurses.render()
sleep(5)
