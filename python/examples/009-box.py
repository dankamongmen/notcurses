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

COLORS = (
    (None, None),
    (None, nc.rgb(128, 128, 128)),          # default on grey
    (nc.rgb(255, 0, 0), None),              # red on default
    (nc.rgb(0, 255, 0), nc.rgb(0, 0, 255)), # green on blue
)

SY = 7
SX = 10

for y, (fg, bg) in enumerate(COLORS):
    for x, box_chars in enumerate(BOX_CHARS):
        plane.cursor_move_yx(y * SY + 1, x * SX + 1);
        nc.box(
            plane, (y + 1) * SY - 1, (x + 1) * SX - 1,
            box_chars,
            fg=fg, bg=bg,
            # ctlword=0x1f9
        )

notcurses.render()
sleep(5)
