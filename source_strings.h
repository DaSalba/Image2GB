/**
 * @file  source_strings.h
 * @brief Premade strings containing C source code for an image asset in GBDK-2020 - header.
 */

#ifndef IMAGE2GB_SOURCE_STRINGS_H_INCLUDED
#define IMAGE2GB_SOURCE_STRINGS_H_INCLUDED

// CONSTANTS ///////////////////////////////////////////////////////////////////

/** String that stores a premade .h header of a GBDK-2020 image asset, filled
 *  with format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_H "/**\n\
 * @file  %s.h\n\
 * @brief %s, exported by Image2GB for use with GBDK-2020 - header.\n\
 *\n\
 * Unique tiles  : %u\n\
 * Total tiles   : %u\n\
 * Size (tiles)  : %ux%u\n\
 * Size (pixels) : %ux%u\n\
 * Bank          : %u\n\
 */\n\
\n\
#pragma once\n\
\n\
%s#include <gb/gb.h>\n\
\n\
%sBANKREF_EXTERN(GAME_BACKGROUNDS_%s)\n\
\n\
// CONSTANTS ///////////////////////////////////////////////////////////////////\n\
\n\
#define GAME_BACKGROUNDS_%s_TILES %uU /**< How many unique tiles this background has. */\n\
\n\
#define GAME_BACKGROUNDS_%s_SIZE_X %uU /**< Width of this background, in 8x8 tiles. */\n\
#define GAME_BACKGROUNDS_%s_SIZE_Y %uU /**< Height of this background, in 8x8 tiles. */\n\
\n\
/** %s (data), exported by Image2GB for use with GBDK-2020.\n\
 */\n\
extern const unsigned char BackgroundData%s[];\n\
\n\
/** %s (map), exported by Image2GB for use with GBDK-2020.\n\
 */\n\
extern const unsigned char BackgroundMap%s[];\n"

/** String that stores part 1 of a premade .c source of a GBDK-2020 image asset,
 *  filled with format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_C_1 "/**\n\
 * @file  %s.c\n\
 * @brief %s, exported by Image2GB for use with GBDK-2020 - data.\n\
 *\n\
 * Unique tiles  : %u\n\
 * Total tiles   : %u\n\
 * Size (tiles)  : %ux%u\n\
 * Size (pixels) : %ux%u\n\
 * Bank          : %u\n\
 */\n\
\n\
#include \"%s.h\"\n\
\n\
%s#include <gb/gb.h>\n\
\n\
%sBANKREF(GAME_BACKGROUNDS_%s)\n\
\n\
// CONSTANTS ///////////////////////////////////////////////////////////////////\n\
\n\
const unsigned char BackgroundData%s[] =\n\
{\n"

/** String that stores part 2 of a premade .c source of a GBDK-2020 image asset,
 *  filled with format specifiers, ready to get sent to printf.
 */
#define IMAGE2GB_SOURCE_STRING_C_2 "};\n\
\n\
const unsigned char BackgroundMap%s[] =\n\
{\n"

#endif // IMAGE2GB_SOURCE_STRINGS_H_INCLUDED
