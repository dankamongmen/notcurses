.DELETE_ON_ERROR:
.PHONY: all book serve clean
.DEFAULT_GOAL:=all

SRC:=$(wildcard src/*.c)
BIN:=$(foreach src, $(SRC), $(addprefix book/,$(notdir $(basename $(src)))))

all: $(BIN)

serve: book
	mdbook serve

book: $(SRC)
	mdbook build

book/%: src/%.c
	@mkdir -p $(@D)
	$(CC) -o $@ $< -lnotcurses

clean:
	rm -rf $(BIN) book
