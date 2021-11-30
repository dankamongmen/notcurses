import re
from cffi import FFI
ffibuild = FFI()

ffibuild.set_source(
    "_notcurses",
    """
    #include <notcurses/direct.h>
    #include <notcurses/notcurses.h>
    """,
    libraries=["notcurses"],
)

with open('include/notcurses/notcurses.h') as fp:
    lines = fp.read().splitlines()
    # remove initial #includes and #ifdefs
    first = next(i for i, line in enumerate(lines) if 'notcurses_version' in line)
    del lines[:first]
    # remove corresponding #endifs
    last = next(i for i, line in enumerate(lines) if '#undef' in line)
    del lines[last:]

# same with direct.h
with open('include/notcurses/direct.h') as fp:
    direct_lines = fp.read().splitlines()
    first = next(i for i, line in enumerate(direct_lines) if '#define ALLOC' in line)
    del direct_lines[:first+1]
    last = next(i for i, line in enumerate(direct_lines) if '#undef' in line)
    del direct_lines[last:]

# and also nckeys.key
with open('include/notcurses/nckeys.h') as fp:
    nckeys_lines = fp.read().splitlines()
    first = next(i for i, line in enumerate(nckeys_lines) if '#define NCKEY_' in line)
    del nckeys_lines[:first]
    last = next(i for i, line in enumerate(nckeys_lines) if '#ifdef' in line)
    del nckeys_lines[last:]

# turn static function defs into declarations, remove deprecation declarations
lines = '\n'.join(lines + direct_lines + nckeys_lines)
lines = re.sub(r'(?m)(^static (.*[^;]\n)+.*)\(\(deprecated\)\);', r'', lines)
lines = re.sub(r'(?m)(^(ALLOC |__attribute__ .*)?static (.*[^{]\n)+.*)\{$(?s:.*?^\}$)', r'\1;', lines)

# fix missing struct definitions
# it doesn't actually matter what these are typedef'd to
lines = 'typedef ... sigset_t;\ntypedef ... va_list;\n' + lines

# remove various compiler/preprocessor hints that cffi doesn't understand
lines = re.sub(r'\b(API|RESTRICT|ALLOC|static) ', '', lines)
lines = lines.replace('__attribute__ ((unused))', '')
lines = re.sub(r'__attribute(__)? \(\(.*\)\)', '', lines)

# fix up #define constants
# TODO: parse macro functions / struct initializers into C or python functions
def define(m):
    if re.match(r'\s*(0x)?[a-fA-F0-9](u(ll)?)?', m[2]):
        return m[0]
    elif '{' not in m[2]:
        return m[1] + ' ...'
    return ''
lines = re.sub(r'(?m)^(#define [^ (]+) ((.*\\$\n)*.*$)', define, lines)
lines = re.sub(r'(?m)^(#define [^)\n]+\)) (.*\\$\n)*.*$', r'', lines)

# override=True so that the multiple definitions/declarations don't error out
ffibuild.cdef(lines, override=True)

if __name__ == "__main__":
    ffibuild.compile(verbose=True)
