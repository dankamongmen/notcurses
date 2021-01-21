# Release testing

## Unit tests

Run unit tests (`make && make test`):
* In each multimedia configuration (`ffmpeg`, `oiio`, `none`)
* With `LANG` set to `fr_FR.UTF-8` (to test comma as decimal separator)
* With `LANG` set to `C` (to test ASCII mode, necessary for debuilder)
* All must pass

## Manual tests

Run, using `valgrind --tool=memcheck --leak-check=full`:
* `notcurses-demo` in each of the three multimedia configurations
* `notcurses-demo` with `USE_QRCODEGEN=off`
* `notcurses-demo` in ASCII mode (`export LANG=C`)
* `notcurses-input`
* `ncplayer` with each scaling mode and an image + video, in three
   terminal geometries: square, tall, wide
* `notcurses-demo` with margins
* `notcurses-demo` with FPS plot and HUD up
* Play a game of `nctetris`
* Run each PoC binary, including `ncpp_build` and `ncpp_build_exceptions`
