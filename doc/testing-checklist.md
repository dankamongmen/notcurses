# Release testing

## Unit tests

Run unit tests (`make && make test`):
* In each multimedia configuration (`ffmpeg`, `oiio`, `none`)
* With LANG set to `fr_FR.UTF-8` (to test comma as decimal separator)
* All must pass

## Manual tests

Run, using `valgrind --tool=memcheck --leak-check=full`:
* `notcurses-demo` in each of the three multimedia configurations
* `notcurses-demo` with `USE_QRCODEGEN=off`
* `notcurses-ncreel`
* `notcurses-input`
* `notcurses-view` with each scaling mode and an image + video, in three
   terminal geometries: square, tall, wide
* `notcurses-demo` with margins
* `notcurses-demo` with FPS plot and HUD up
* Play a game of `notcurses-tetris`
* Run each PoC binary, including `ncpp_build` and `ncpp_build_exceptions`
