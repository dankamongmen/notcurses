% notcurses_metric(3)
% nick black <nickblack@linux.com>
% v2.3.5

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
#define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - ncstrwidth(x) + (cols)))
#define PREFIXFMT(x) NCMETRICFWIDTH((x), PREFIXCOLUMNS), (x)
#define IPREFIXFMT(x) NCMETRIXFWIDTH((x), IPREFIXCOLUMNS), (x)
#define BPREFIXFMT(x) NCMETRICFWIDTH((x), BPREFIXCOLUMNS), (x)
```

**const char* ncmetric(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***, unsigned ***mult***, int ***uprefix***);**

**static inline const char* qprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

**static inline const char* iprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

**static inline const char* bprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

# DESCRIPTION

**ncmetric** (and the helper wrappers **qprefix** and **bprefix**) accept
very large (or very small) non-negative numbers, and prepare formatted output
of a maximum width using metric suffixes. The suffix can represent arbitrary
amounts of growth, but is designed for 1000 (**PREFIX**) or 1024
(**IPREFIX**). 1024 is used for "digital units of information", i.e. kibibytes
and gibibits. **ncmetric** supports the large suffixes KMGTPEZY (Kilo, Mega,
Giga, Tera, Peta, Exa, Zetta, and Yotta) and the small suffixes mµnpfazy
(Milli, Micro, Nano, Pico, Femto, Atto, Zepto, and Yocto). This covers the
range 1e24 (one septillion) through 1e-24, sufficing for all possible values of
a 64-bit **uintmax_t**.

**val** is the value being output, having been scaled by **decimal**.
**decimal** will typically be 1; to represent values less than 1, **decimal**
should be larger than **val**. The output will be written to **buf**, which
must be at least:

* **PREFIXSTRLEN** + 1 bytes for a 1000-based value
* **IPREFIXSTRLEN** + 1 bytes for a 1024-based value
* **BPREFIXSTRLEN** + 1 bytes for a 1024-based value with an 'i' suffix

Three helper functions are provided to simplify these common cases:

```
// Mega, kilo, gigafoo. Use PREFIXSTRLEN + 1 and PREFIXCOLUMNS.
static inline const char*
qprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes sans 'i' suffix. Use IPREFIXSTRLEN + 1.
static inline const char*
iprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1 and BPREFIXCOLUMNS.
static inline const char*
bprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}
```

If **omitdec** is not zero, the decimal point and mantissa will be
omitted if all digits to be displayed would be zero. The decimal point takes
the current locale into account (see **setlocale(3)** and **localeconv(3)**).
**mult** is the relative multiple for each suffix. **uprefix**, if not zero,
will be used as a suffix following any metric suffix.

The maximum number of columns is not directly related to the maximum number of
bytes, since Unicode doesn't necessarily map to single-byte characters
(including the 'µ' micro suffix). The corresponding defines for maximum column
length are:

* **PREFIXCOLUMNS** (7)
* **IPREFIXCOLUMNS** (8)
* **BPREFIXCOLUMNS** (9)

In general, the maximum-width output will take the form **CCC.mmMu**, where C
are digits of the characteristic (up to ceil(log10(**mult**)) digits), the
decimal point follows, m are digits of the mantissa (up to 2), M is the metric
suffix, and u is the **uprefix**. The minimum-width output will take the form
**C**. This minimal form can occur if **omitdec** is non-zero and a
single-column value such as 5 is passed for **val**.

Three more defines are provided to simplify formatted fixed-width output using
the results of **ncmetric**. Each of these macros accepts a character buffer
holding the result of the call, and expand to *two* arguments:

* **PREFIXFMT(x)**
* **IPREFIXFMT(x)**
* **BPREFIXFMT(x)**

These can be used in e.g. the following ungainly fashion:

**ncplane_printf(n, "%*s", PREFIXFMT(buf));**

to ensure that the output is always **PREFIXCOLUMNS** wide.

# RETURN VALUES

**NULL** if input parameters were invalid. Otherwise, a pointer to **buf**,
filled in with the formatted output.

# EXAMPLES

**ncmetric(0, 1, buf, 0, 1000, '\0')**: "0.00".

**ncmetric(0, 1, buf, 1, 1000, '\0')**: "0".

**ncmetric(1023, 1, buf, 1, 1000, '\0')**: "1.02K".

**ncmetric(1023, 1, buf, 1, 1024, 'i')**: "1023".

**ncmetric(1024, 1, buf, 1, 1024, 'i')**: "1Ki".

**ncmetric(4096, 1, buf, 0, 1024, 'i')**: "4.00Ki".

**ncmetric(0x7fffffffffffffff, 1, buf, 0, 1000, '\0')**: "9.22E".

**ncmetric(0xffffffffffffffff, 1, buf, 0, 1000, '\0')**: "18.45E".

# BUGS

This function is difficult to understand.

# SEE ALSO

**localeconv(3)**,
**notcurses(3)**,
**notcurses_output(3)**,
**setlocale(3)**
