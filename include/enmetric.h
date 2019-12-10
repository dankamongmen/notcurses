#ifndef NOTCURSES_ENMETRIC
#define NOTCURSES_ENMETRIC

#ifdef __cplusplus
extern "C" {
#else
#define RESTRICT restrict
#endif

#define PREFIXSTRLEN 7  // Does not include a '\0' (xxx.xxU)
#define IPREFIXSTRLEN 8 //  Does not include a '\0' (xxxx.xxU)
#define BPREFIXSTRLEN 9  // Does not include a '\0' (xxxx.xxUi), i == prefix

// A bit of the nasties here to stringize our preprocessor tokens just now
// #defined, making them usable as printf(3) specifiers.
#define STRHACK1(x) #x
#define STRHACK2(x) STRHACK1(x)
#define PREFIXFMT "%" STRHACK2(PREFIXSTRLEN) "s"
#define IPREFIXFMT "%" STRHACK2(IPREFIXSTRLEN) "s"
#define BPREFIXFMT "%" STRHACK2(BPREFIXSTRLEN) "s"

// Takes an arbitrarily large number, and prints it into a fixed-size buffer by
// adding the necessary SI suffix. Usually, pass a |[B]PREFIXSTRLEN+1|-sized
// buffer to generate up to [B]PREFIXSTRLEN characters. The characteristic can
// occupy up through |mult-1| characters (3 for 1000, 4 for 1024). The mantissa
// can occupy either zero or two characters.
//
// Floating-point is never used, because an IEEE758 double can only losslessly
// represent integers through 2^53-1.
//
// 2^64-1 is 18446744073709551615, 18.45E(xa). KMGTPEZY thus suffice to handle
// a 89-bit uintmax_t. Beyond Z(etta) and Y(otta) lie lands unspecified by SI.
//
// val: value to print
// decimal: scaling. '1' if none has taken place.
// buf: buffer in which string will be generated
// omitdec: inhibit printing of all-0 decimal portions
// mult: base of suffix system (almost always 1000 or 1024)
// uprefix: character to print following suffix ('i' for kibibytes basically).
//   only printed if suffix is actually printed (input >= mult).
const char *enmetric(uintmax_t val, unsigned decimal, char *buf, int omitdec,
                     unsigned mult, int uprefix);

// Mega, kilo, gigabytes. Use PREFIXSTRLEN + 1.
static inline const char *
qprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char *
bprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1024, 'i');
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif
