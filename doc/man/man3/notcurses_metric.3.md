% notcurses_metric(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_metric - fixed-width numeric output with metric suffixes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCPREFIXCOLUMNS 7
#define NCIPREFIXCOLUMNS 8
#define NCBPREFIXCOLUMNS 9
#define NCPREFIXSTRLEN (NCPREFIXCOLUMNS + 1)
#define NCIPREFIXSTRLEN (NCIPREFIXCOLUMNS + 1)
#define NCBPREFIXSTRLEN (NCBPREFIXCOLUMNS + 1)
#define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - ncstrwidth(x) + (cols)))
#define NCPREFIXFMT(x) NCMETRICFWIDTH((x), NCPREFIXCOLUMNS), (x)
#define NCIPREFIXFMT(x) NCMETRIXFWIDTH((x), NCIPREFIXCOLUMNS), (x)
#define NCBPREFIXFMT(x) NCMETRICFWIDTH((x), NCBPREFIXCOLUMNS), (x)
```

**const char* ncnmetric(uintmax_t ***val***, size_t s, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***, unsigned ***mult***, int ***uprefix***);**

**static inline const char* ncqprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

**static inline const char* nciprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

**static inline const char* ncbprefix(uintmax_t ***val***, uintmax_t ***decimal***, char* ***buf***, int ***omitdec***);**

# DESCRIPTION

**ncmetric** (and the helper wrappers **qprefix** and **bprefix**) accept
very large (or very small) non-negative numbers, and prepare formatted output
of a maximum width using metric suffixes. The suffix can represent arbitrary
amounts of growth, but is designed for 1000 (**NCPREFIX**) or 1024
(**NCIPREFIX**). 1024 is used for "digital units of information", i.e. kibibytes
and gibibits. **ncmetric** supports the large suffixes KMGTPEZY (Kilo, Mega,
Giga, Tera, Peta, Exa, Zetta, and Yotta) and the small suffixes mµnpfazy
(Milli, Micro, Nano, Pico, Femto, Atto, Zepto, and Yocto). This covers the
range 1e24 (one septillion) through 1e-24, sufficing for all possible values of
a 64-bit **uintmax_t**.

***val*** is the value being output, having been scaled by ***decimal***.
***decimal*** will typically be 1; to represent values less than 1, ***decimal***
should be larger than ***val***. The output will be written to ***buf***, which
must be at least:

* **NCPREFIXSTRLEN** + 1 bytes for a 1000-based value
* **NCIPREFIXSTRLEN** + 1 bytes for a 1024-based value
* **NCBPREFIXSTRLEN** + 1 bytes for a 1024-based value with an 'i' suffix

***s*** is the maximum output size, including '\0', used in the same
fashion as **snprintf(3)** (which **ncnmetric** calls).

Three helper functions are provided to simplify these common cases:

```
// Mega, kilo, gigafoo. Use NCPREFIXSTRLEN + 1 and NCPREFIXCOLUMNS.
static inline const char*
ncqprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes sans 'i' suffix. Use NCIPREFIXSTRLEN + 1.
static inline const char*
nciprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, '\0');
}

// Mibi, kebi, gibibytes. Use NCBPREFIXSTRLEN + 1 and NCBPREFIXCOLUMNS.
static inline const char*
ncbprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}
```

If **omitdec** is not zero, the decimal point and mantissa will be
omitted if all digits to be displayed would be zero. The decimal point takes
the current locale into account (see **setlocale(3)** and **localeconv(3)**).
***mult*** is the relative multiple for each suffix. ***uprefix***, if not zero,
will be used as a suffix following any metric suffix.

The maximum number of columns is not directly related to the maximum number of
bytes, since Unicode doesn't necessarily map to single-byte characters
(including the 'µ' micro suffix). The corresponding defines for maximum column
length are:

* **NCPREFIXCOLUMNS** (7)
* **NCIPREFIXCOLUMNS** (8)
* **NCBPREFIXCOLUMNS** (9)

In general, the maximum-width output will take the form **CCC.mmMu**, where C
are digits of the characteristic (up to ceil(log10(**mult**)) digits), the
decimal point follows, m are digits of the mantissa (up to 2), M is the metric
suffix, and u is the ***uprefix***. The minimum-width output will take the form
**C**. This minimal form can occur if ***omitdec*** is non-zero and a
single-column value such as 5 is passed for ***val***.

Three more defines are provided to simplify formatted fixed-width output using
the results of **ncmetric**. Each of these macros accepts a character buffer
holding the result of the call, and expand to *two* arguments:

* **NCPREFIXFMT(x)**
* **NCIPREFIXFMT(x)**
* **NCBPREFIXFMT(x)**

These can be used in e.g. the following ungainly fashion:

**ncplane_printf(n, "%*s", NCPREFIXFMT(buf));**

to ensure that the output is always **NCPREFIXCOLUMNS** wide.

# RETURN VALUES

**NULL** if input parameters were invalid. Otherwise, a pointer to ***buf***,
filled in with the formatted output.

# EXAMPLES

**ncnmetric(0, INT_MAX, buf, 0, 1000, '\0')**: "0.00".

**ncnmetric(0, INT_MAX, buf, 1, 1000, '\0')**: "0".

**ncnmetric(1023, INT_MAX, buf, 1, 1000, '\0')**: "1.02K".

**ncnmetric(1023, INT_MAX, buf, 1, 1024, 'i')**: "1023".

**ncnmetric(1024, INT_MAX, buf, 1, 1024, 'i')**: "1Ki".

**ncnmetric(4096, INT_MAX, buf, 0, 1024, 'i')**: "4.00Ki".

**ncnmetric(0x7fffffffffffffff, INT_MAX, buf, 0, 1000, '\0')**: "9.22E".

**ncnmetric(0xffffffffffffffff, INT_MAX, buf, 0, 1000, '\0')**: "18.45E".

# BUGS

This function is difficult to understand.

# SEE ALSO

**localeconv(3)**,
**notcurses(3)**,
**notcurses_output(3)**,
**setlocale(3)**,
**snprintf(3)**
