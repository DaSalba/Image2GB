/**
 * @file source_strings.h
 * @brief Premade strings containing C source code for an image asset in GBDK. (header)
 */

#ifndef IMAGE2GB_SOURCE_STRINGS_H_INCLUDED
#define IMAGE2GB_SOURCE_STRINGS_H_INCLUDED

// DEFINITIONS /////////////////////////////////////////////////////////////////

/** String that stores a premade .h header of a GBDK image asset, filled with
 *  format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_H "/**\n\
 * @file %s%s\n\
 * @brief %s, exported by Image2GB for use with GBDK. (header)\n\
 *\n\
 * Unique tiles  : %u\n\
 * Total tiles   : %u\n\
 * Size (tiles)  : %ux%u\n\
 * Size (pixels) : %ux%u\n\
 */\n\
\n\
#ifndef GAME_GRAPHICS_BACKGROUNDS_%s_H_INCLUDED\n\
#define GAME_GRAPHICS_BACKGROUNDS_%s_H_INCLUDED\n\
\n\
// DEFINITIONS /////////////////////////////////////////////////////////////////\n\
\n\
%s#define GAME_BANK_ROM_GRAPHICS_BACKGROUNDS_%s %u /**< ROM bank where this background is stored. */\n\
\n\
#define GAME_BACKGROUNDS_%s_TILES %uU /**< Data size of this background, in unique tiles. */\n\
\n\
#define GAME_BACKGROUNDS_%s_SIZE_X %uU /**< Width of this background, in tiles (1 tile = 8x8 pixels). */\n\
\n\
#define GAME_BACKGROUNDS_%s_SIZE_Y %uU /**< Height of this background, in tiles (1 tile = 8x8 pixels). */\n\
\n\
// VARIABLES ///////////////////////////////////////////////////////////////////\n\
\n\
/** %s (data), exported by Image2GB for use with GBDK.\n\
 */\n\
extern const unsigned char BkgData%s[];\n\
\n\
/** %s (map), exported by Image2GB for use with GBDK.\n\
 */\n\
extern const unsigned char BkgMap%s[];\n\
\n\
#endif // GAME_GRAPHICS_BACKGROUNDS_%s_H_INCLUDED"

/** String that stores part 1 of a premade .c source of a GBDK image asset,
 *  filled with format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_C_1 "/**\n\
 * @file %s%s\n\
 * @brief %s, exported by Image2GB for use with GBDK. (implementation)\n\
 *\n\
 * Unique tiles  : %u\n\
 * Total tiles   : %u\n\
 * Size (tiles)  : %ux%u\n\
 * Size (pixels) : %ux%u\n\
 */\n\
\n\
#include \"%s%s\"\n\
\n\
// VARIABLES ///////////////////////////////////////////////////////////////////\n\
\n\
const unsigned char BkgData%s[] =\n\
{\n"

/** String that stores part 2 of a premade .c source of a GBDK image asset,
 *  filled with format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_C_2 "};\n\
\n\
const unsigned char BkgMap%s[] =\n\
{\n"

#endif // IMAGE2GB_SOURCE_STRINGS_H_INCLUDED
