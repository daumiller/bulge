#ifndef BULGE_ENDIAN_HEADER
#define BULGE_ENDIAN_HEADER

#ifdef BULGE_LITTLE_ENDIAN
// basic versions, if you don't have __builtin_bswapXX()
// #define BULGE_ENDIAN32(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | (((x) & 0x000000FF) << 24))
// #deffine BULGE_ENDIAN16(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))
#define BULGE_ENDIAN32(x) __builtin_bswap32(x)
#define BULGE_ENDIAN16(x) __builtin_bswap16(x)
#else // ifdef BULGE_LITTLE_ENDIAN
#define BULGE_ENDIAN32(x) (x)
#define BULGE_ENDIAN16(x) (x)
#endif // ifdef BULGE_LITTLE_ENDIAN

#endif // ifndef BULGE_ENDIAN_HEADER
