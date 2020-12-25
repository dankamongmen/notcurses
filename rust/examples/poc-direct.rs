//! based on the proof of concept at ../../src/poc/direct.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let ncd = NcDirect::new();

    let dimy = ncd.dim_y();
    let dimx = ncd.dim_x();
    for _ in 0..dimy {
        for _ in 0..dimx {
            printf!("X");
        }
    }
    ncd.flush()?;

    ncd.fg_rgb(0xff8080)?;
    ncd.styles_on(NCSTYLE_STANDOUT)?;
    printf!(" erp erp \n");
    ncd.fg_rgb(0x80ff80)?;
    printf!(" erp erp \n");
    ncd.styles_off(NCSTYLE_STANDOUT)?;
    printf!(" erp erp \n");
    ncd.fg_rgb(0xff8080)?;
    printf!(" erp erp \n");
    ncd.cursor_right(dimx / 2)?;
    ncd.cursor_up(dimy / 2)?;
    printf!(" erperperp! \n");

    let (mut y, x);

    if let Some((_y, _x)) = ncd.cursor_yx() {
        y = _y;
        x = _x;
        printf!("\n\tRead cursor position: y: %d x: %d\n", y, x);

        y += 2;
        while y > 3 {
            let up = if y >= 3 { 3 } else { y };
            ncd.cursor_up(up)?;
            ncd.flush()?;
            y -= up;

            let newy;
            if let Some((_y, _)) = ncd.cursor_yx() {
                newy = _y;
            } else {
                break;
            }

            if newy != y {
                eprintln!("Expected {}, got {}", y, newy);
                break;
            }
            printf!("\n\tRead cursor position: y: %d x: %d\n", newy, x);
            y += 2;
        }
    } else {
        return Err(NcError::new(-10, "Couldn't read cursor position."));
    }

    ncd.stop()?;
    Ok(())
}
