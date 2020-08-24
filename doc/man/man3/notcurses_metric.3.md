% notcurses_metric(3)
% nick black <nickblack@linux.com>
% v1.6.17

# NAME

notcurses_metric - fixed-width numeric output with metric suffixes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define PREFIXCOLUMNS 7
#define IPREFIXCOLUMNS 8
#define BPREFIXCOLUMNS 9
#define PREFIXSTRLEN (PREFIXCOLUMNS + 1)
#define IPREFIXSTRLEN (IPREFIXCOLUMNS + 1)
#define BPREFIXSTRLEN (BPREFIXCOLUMNS + 1)
#define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - mbswidth(x) + (cols)))
#define PREFIXFMT(x) NCMETRICFWIDTH((x), PREFIXCOLUMNS), (x)
#define IPREFIXFMT(x) NCMETRIXFWIDTH((x), IPREFIXCOLUMNS), (x)
#define BPREFIXFMT(x) NCMETRICFWIDTH((x), BPREFIXCOLUMNS), (x)
```

**const char* ncmetric(uintmax_t val, uintmax_t decimal, char* buf, int omitdec, unsigned mult, int uprefix);**

```c
// Mega, kilo, gigafoo. Use PREFIXSTRLEN + 1.
static inline const char*
qprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char*
bprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}
```

# DESCRIPTION

**ncmetric** (and the helper wrappers **qprefix** and **bprefix**) accept
very large (or very small) non-negative numbers, and prepare formatted output
of a maximum width using metric suffixes. The suffix can represent arbitrary
amounts of growth, but is designed for 1000 (**PREFIX**) or 1024
(**IPREFIX**). 1024 is used for "digital units of information", i.e. kibibytes
and kilobytes. **ncmetric** supports the large suffixes KMGTPEZY (Kilo, Mega,
Giga, Tera, Peta, Exa, Zetta, and Yotta) and the small suffixes mµnpfazy
(Milli, Micro, Nano, Pico, Femto, Atto, Zepto, and Yocto). This covers the
range 10^-24 through 10^24. As **uintmax_t** is typically only 64 bits, this
covers the entirety of its range.

**val** is the value being output, having been scaled by **decimal**.
**decimal** will typically be 1; to represent values less than 1, **decimal**
should be larger than **val**. The output will be written to **buf**, which
must be at least:

* **PREFIXSTRLEN** + 1 bytes for a 1000-based value
* **IPREFIXSTRLEN** + 1 bytes for a 1024-based value
* **BPREFIXSTRLEN** + 1 bytes for a 1024-based value with an 'i' suffix

If **omitdec** is not zero, the decimal point and mantissa will be
omitted if all digits to be displayed would be zero. The decimal point takes
the current locale into account (see **setlocale(3)** and **localeconv(3)**).
**mult** is the relative multiple for each suffix. **uprefix**, if not zero,
will be used as a suffix following any metric suffix.

In general, the maximum-width output will take the form:

   CCC.mmMu

Where C are digits of the characteristic (up to ceil(log10(**mult**)) digits),
the decimal point follows, m are digits of the mantissa (up to 2), M is the
metric suffix, and u is the **uprefix**. The minimum-width output will take
the form:

   C

This can occur if **omitdec** is non-zero and a value such as 5 is passed
for **val**.

# RETURN VALUES

**NULL** if input parameters were invalid. Otherwise, a pointer to **buf**,
filled in with the formatted output.

# SEE ALSO

**localeconv(3)**,
**notcurses(3)**,
**notcurses_plane(3)**,
**setlocale(3)**
