import re
import glob
import os
import sys

match_static_inline = re.compile(r'static inline (.*?)\n([\S\s][^;]*?){')

def generate_ffi(notcurses_dir):
    lines = []
    for file in glob.glob(os.path.join(notcurses_dir, "include", "notcurses", "*.h")):
        with open(file, 'r') as f:
            content = f.read()
            finds = re.findall(match_static_inline, content)
            for find in finds:
                lines.append(find[0] + ' ' + find[1] + ';')

    lines.sort()

    with open(os.path.join(notcurses_dir, "src", "libffi", "ffi.c"), 'w') as f:
        f.write("// Contains all inline functions in include/notcurses/*.h\n")
        f.write("// This file is auto generated from tools/generate_ffi.py\n")
        f.write("#include <notcurses/notcurses.h>\n")
        f.write("#include <notcurses/direct.h>\n\n")
        f.write("#include <notcurses/nckeys.h>\n\n")

        for line in lines:
            line = line.replace("RESTRICT", "restrict")
            f.write(line)
            f.write('\n')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: generate_ffi.py notcurses-dir")
    else:
        generate_ffi(sys.argv[1])
