#!/usr/bin/env bash
#
# handy script that shows the last changes made by Nick in the rust bindings

RUST_DIR="$(git rev-parse --show-toplevel)/rust"

# …in /src & /examples
for file in $(find "$RUST_DIR/src/" "$RUST_DIR/examples" -name '*.rs'); do
	git blame -fn $file 2>&1 | grep "(nick black" | grep -v "no such path"; 
done

