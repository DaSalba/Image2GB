/**
 * @file  source_strings.h
 * @brief Premade strings containing C source code for an image asset in GBDK-2020 - header.
 */

#pragma once

// CONSTANTS ///////////////////////////////////////////////////////////////////

/** String that stores a premade .h header of a GBDK-2020 image asset, filled
 *  with format specifiers, ready to get sent to printf().
 */
static const char* SsourceStringH =
    "/**\n"
    " * @file  %s.h\n"
    " * @brief %s, exported by Image2GB for use with GBDK-2020 - header.\n"
    " *\n"
    " * Unique tiles  : %u\n"
    " * Total tiles   : %u\n"
    " * Size (tiles)  : %ux%u\n"
    " * Size (pixels) : %ux%u\n"
    " * Bank          : %u\n"
    " */\n"
    "\n"
    "#pragma once\n"
    "\n"
    "%s\n"
    "\n"
    "// CONSTANTS ///////////////////////////////////////////////////////////////////\n"
    "\n"
    "#define BACKGROUND_%s_TILES %uU /**< How many unique tiles this background has. */\n"
    "\n"
    "#define BACKGROUND_%s_SIZE_X %uU /**< Width of this background, in 8x8 tiles (columns). */\n"
    "#define BACKGROUND_%s_SIZE_Y %uU /**< Height of this background, in 8x8 tiles (rows). */\n"
    "\n"
    "/** %s (data), exported by Image2GB for use with GBDK-2020.\n"
    " */\n"
    "extern const uint8_t BackgroundData%s[];\n"
    "\n"
    "/** %s (map), exported by Image2GB for use with GBDK-2020.\n"
    " */\n"
    "extern const uint8_t BackgroundMap%s[];\n";

/** String that stores part 1 of a premade .c source of a GBDK-2020 image asset,
 *  filled with format specifiers, ready to get sent to printf().
 */
static const char* SsourceStringC1 =
    "/**\n"
    " * @file  %s.c\n"
    " * @brief %s, exported by Image2GB for use with GBDK-2020 - data.\n"
    " *\n"
    " * Unique tiles  : %u\n"
    " * Total tiles   : %u\n"
    " * Size (tiles)  : %ux%u\n"
    " * Size (pixels) : %ux%u\n"
    " * Bank          : %u\n"
    " */\n"
    "\n"
    "#include \"%s.h\"\n"
    "\n"
    "%s\n"
    "\n"
    "// CONSTANTS ///////////////////////////////////////////////////////////////////\n"
    "\n"
    "const uint8_t BackgroundData%s[] =\n"
    "{\n";

/** String that stores part 2 of a premade .c source of a GBDK-2020 image asset,
 *  filled with format specifiers, ready to get sent to printf().
 */
static const char* SsourceStringC2 =
    "};\n"
    "\n"
    "const uint8_t BackgroundMap%s[] =\n"
    "{\n";
