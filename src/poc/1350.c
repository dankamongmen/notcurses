#include <locale.h>
#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdlib.h>

static struct ncplane *make_status_line_plane(struct ncplane *std_plane) {
  struct ncplane_options opts = {0};
  opts.rows = 1;
  opts.cols = ncplane_dim_x(std_plane);
  opts.y = ncplane_dim_y(std_plane) - 1;
  opts.x = 0;

  nccell c = CELL_CHAR_INITIALIZER(' ');
  cell_set_bg_rgb8(&c, 0x20, 0x20, 0x20);

  struct ncplane *status_line_plane = ncplane_create(std_plane, &opts);
  ncplane_set_base_cell(status_line_plane, &c);
  return status_line_plane;
}

static void print_status_line(struct ncplane *plane) {
  ncplane_putstr_aligned(plane, 0, NCALIGN_LEFT, " ?: help");
}

static void print_help(struct ncplane *plane) {
  ncplane_erase(plane);
  ncplane_putstr_aligned(plane, 0, NCALIGN_LEFT,
                         " g: jump to address   q: quit");
  ncplane_putstr_aligned(plane, 0, NCALIGN_RIGHT,
                         "press any key to close help ");
}

static int query_address(struct notcurses *nc, struct ncplane *plane,
                         const char *question, uint16_t *address) {
  int y = 0;
  int x = strlen(question) + 1;
  ncplane_translate(plane, NULL, &y, &x);

  ncplane_printf_yx(plane, 0, 1, question);
  notcurses_cursor_enable(nc, y, x);

  struct ncplane_options plane_opts = {0};
  plane_opts.rows = 1;
  plane_opts.cols = 4;
  plane_opts.y = 0;
  plane_opts.x = strlen(question) + 1;

  struct ncplane *reader_plane = ncplane_create(plane, &plane_opts);
  uint64_t channels = 0;
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(reader_plane, "", 0, channels);

  struct ncreader_options reader_opts = {0};
  reader_opts.flags = NCREADER_OPTION_CURSOR;

  struct ncreader *reader = ncreader_create(reader_plane, &reader_opts);
  notcurses_render(nc);

  char32_t c;
  struct ncinput input;
  while ((c = notcurses_getc_blocking(nc, &input)) != NCKEY_ENTER &&
         c != NCKEY_ESC) {
    if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
        ('A' <= c && c <= 'F') || c == NCKEY_BACKSPACE) {
      ncreader_offer_input(reader, &input);
      notcurses_render(nc);
    }
  }

  char *contents;
  ncreader_destroy(reader, &contents);
  const int is_success =
      (c != NCKEY_ESC && sscanf(contents, "%4hx", address) == 1) ? 0 : -1;

  free(contents);
  ncplane_erase(plane);

  return is_success;
}

int main(void) {
  setlocale(LC_ALL, "");

  notcurses_options opts = {0};
  opts.flags = NCOPTION_SUPPRESS_BANNERS;

  struct notcurses *nc;
  if ((nc = notcurses_core_init(&opts, NULL)) == NULL) {
    return EXIT_FAILURE;
  }

  struct ncplane *std_plane = notcurses_stdplane(nc);
  struct ncplane *status_line_plane = make_status_line_plane(std_plane);

  ncplane_move_top(status_line_plane);

  struct ncinput input = {0};
  bool quit = false;
  while (!quit) {
    print_status_line(status_line_plane);
    notcurses_render(nc);
    notcurses_getc_blocking(nc, &input);
    switch (input.id) {
    case '?':
      print_help(status_line_plane);
      notcurses_render(nc);
      notcurses_getc_blocking(nc, &input);
      ncplane_erase(status_line_plane);
      break;
    case 'g': {
      uint16_t address;
      query_address(nc, status_line_plane, "Jump to address: $", &address);
    } break;
    case 'q':
      quit = true;
      break;
    }
  }

  notcurses_stop(nc);
}
