/**
 * @file  image_export.h
 * @brief Functionality for exporting a GIMP indexed image to Game Boy data - header + implementation.
 */

#pragma once

#include <libgimp/gimp.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "image2gb.h"
#include "source_strings.h"

// CONSTANTS ///////////////////////////////////////////////////////////////////

#define IMAGE2GB_TILE_SIZE 8 /**< Size of a tile, in pixels (any dimension). */

#define IMAGE2GB_IMAGE_TILES_VRAM_LIMIT 256 /**< How many unique tiles will fit in the Game Boy's VRAM at a time. */

#define IMAGE2GB_IMAGE_SIZE_MIN IMAGE2GB_TILE_SIZE /**< Minimum acceptable image size, in pixels (any dimension). */
#define IMAGE2GB_IMAGE_SIZE_MAX 256                /**< Maximum acceptable image size, in pixels (any dimension). */

#define IMAGE2GB_IMAGE_COLORS 4 /**< How many colors there should be in the palette of an indexed image. */

#define IMAGE2GB_ASSET_NAME_MAX  32 /**< Maximum length of the asset name used for the C variable identifier. */
#define IMAGE2GB_BANK_MAX       255 /**< Last available ROM bank number. */

// DEFINITIONS /////////////////////////////////////////////////////////////////

/** Object that represents a tile in GIMP: a 8x8 pixels square.
 */
typedef unsigned char ImageTile[IMAGE2GB_TILE_SIZE * IMAGE2GB_TILE_SIZE];

/** Object that represents a tile in Game Boy: a 8x8 square with 4-color (2 bit)
 *  pixels. Hence, a tile has 64 * 2 = 128 bits (16 bytes) of data, in 8 rows of
 *  16 bits (2 bytes) each.
 */
typedef struct DataTile
{
	uint16_t row[IMAGE2GB_TILE_SIZE]; /**< Array that stores the 8 pixel rows of this tile. */
	
	gboolean duplicate; /**< Flag for marking this tile as a duplicate of another. */
} DataTile;

// VARIABLES ///////////////////////////////////////////////////////////////////

/** Array that stores all tiles of the image, in Game Boy data format.
 */
static DataTile AdataTiles[(IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)
                           * (IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)];

/** Array that stores the tilemap of the image, in Game Boy data format.
 */
static unsigned int AtileMap[(IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)
                             * (IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)];

static unsigned int UItileWidth;  /**< Horizontal size of the asset, in tiles. */
static unsigned int UItileHeight; /**< Vertical size of the asset, in tiles. */
static unsigned int UItileCount;  /**< Total number of tiles the asset has. */

// FUNCTIONS ///////////////////////////////////////////////////////////////////

/** Tries to export the given image.
 *
 * \param[in] POimage  Pointer to the image to export.
 * \param[in] ErunMode The run mode (interactive or not).
 *
 * \return The procedure run status.
 */
static GimpPDBStatusType
image2gb_export_image(GimpImage* POimage, GimpRunMode ErunMode);

/** Reads the GIMP image and populates the data tile array accordingly.
 *
 * \param[in] POimage  Pointer to the image to parse.
 * \param[in] ErunMode The run mode (interactive or not).
 *
 * \return The procedure run status.
 */
static GimpPDBStatusType
image2gb_read_image_tiles(GimpImage* POimage, GimpRunMode ErunMode);

/** Parses an ImageTile (GIMP format) and computes a DataTile (Game Boy format).
 *
 * \param[in] POimageTile Pointer to the image tile to parse.
 * \param[in] POdataTile  Pointer to the data tile to write.
 */
static void
image2gb_convert_tile(ImageTile* POimageTile, DataTile* POdataTile);

/** Checks all tiles and finds the duplicates, removing them from the tilemap.
 */
static void
image2gb_check_duplicates(void);

/** Writes the output .h header and .c source files containing the image asset.
 *
 * \param[in] ErunMode The run mode (interactive or not).
 *
 * \return The procedure run status.
 */
static GimpPDBStatusType
image2gb_write_files(GimpRunMode ErunMode);

/** Writes the asset tile data to the given file in the format expected by GBDK.
 *
 * \param[in] POfileOut Pointer to the file descriptor to write to.
 */
static void
image2gb_write_tile_data(FILE* POfileOut);

/** Writes the asset tilemap to the given file, in the format expected by GBDK.
 *
 * \param[in] POfileOut Pointer to the file descriptor to write to.
 */
static void
image2gb_write_tilemap(FILE* POfileOut);

////////////////////////////////////////////////////////////////////////////////

static GimpPDBStatusType
image2gb_export_image(GimpImage* POimage, GimpRunMode ErunMode)
{
	GimpPDBStatusType EreturnValue; /**< Return value (PDB status) of this procedure. */
	
	// Compute image statistics.
	UItileWidth = (gimp_image_get_width(POimage) / IMAGE2GB_TILE_SIZE);
	UItileHeight = (gimp_image_get_height(POimage) / IMAGE2GB_TILE_SIZE);
	
	// Try to read the image, and abort if it fails.
	EreturnValue = image2gb_read_image_tiles(POimage, ErunMode);
	
	if (EreturnValue != GIMP_PDB_SUCCESS)
		return EreturnValue;
		
	// Remove unneeded tiles to save VRAM.
	image2gb_check_duplicates();
	
	// Give a warning if the final image will not fit in the Game Boy's VRAM.
	if (UItileCount > IMAGE2GB_IMAGE_TILES_VRAM_LIMIT)
	{
		char* SformattedMessage = g_strdup_printf("WARNING: this image has %u unique tiles. "
		                                          "The Game Boy video memory can only fit up to %d at the same time "
		                                          "(384 using a hack). It will probably give errors.",
		                                          UItileCount, IMAGE2GB_IMAGE_TILES_VRAM_LIMIT);
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
	}
	
	// Finally, try to write the output .h and .c files.
	EreturnValue = image2gb_write_files(ErunMode);
	
	return EreturnValue;
}

static GimpPDBStatusType
image2gb_read_image_tiles(GimpImage* POimage, GimpRunMode ErunMode)
{
	GimpDrawable** Adrawables; /**< Array of selected drawables (NULL if none). */
	
	GeglBuffer* PObuffer; /**< Buffer for accessing pixel data. */
	
	ImageTile OimageTile; /**< Array that stores the GIMP pixels of a tile (8x8). */
	
	// Get the selected drawables from the image.
	Adrawables = gimp_image_get_selected_drawables(POimage);
	
	if (!Adrawables)
	{
		// This should not really happen, but just in case.
		report_message(ErunMode, "No valid layer selected");
		
		return GIMP_PDB_EXECUTION_ERROR;
	}
	
	PObuffer = gimp_drawable_get_buffer(Adrawables[0]);
	
	// Loop through all tiles of the GIMP image.
	for (unsigned int row = 0; row < UItileHeight; row++)
	{
		for (unsigned int col = 0; col < UItileWidth; col++)
		{
			// Define the rectangle for the current tile.
			GeglRectangle GRrectangle =
			{
				.x      = IMAGE2GB_TILE_SIZE * col,
				.y      = IMAGE2GB_TILE_SIZE * row,
				.width  = IMAGE2GB_TILE_SIZE,
				.height = IMAGE2GB_TILE_SIZE
			};
			
			// Read the pixels for this tile (values are color indices, 0-3).
			gegl_buffer_get(PObuffer,
			                &GRrectangle, 1.0,
			                gimp_drawable_get_format(Adrawables[0]),
			                OimageTile,
			                GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
			                
			// Call this other function which will parse and store that tile.
			image2gb_convert_tile(&OimageTile, AdataTiles + ((row * UItileWidth) + col));
		}
	}
	
	// Cleanup.
	g_free(Adrawables);
	g_object_unref(PObuffer);
	
	return GIMP_PDB_SUCCESS;
}

static void
image2gb_convert_tile(ImageTile* POimageTile, DataTile* POdataTile)
{
	// Visual explanation: right now we are processing a single tile, which is a
	// square 8x8 pixels area of the image, 64 pixels in total. We have 2
	// variable types:
	//
	// 1- ImageTile, filled with 64 char that contain the values of
	//    every pixel of this tile. Every pixel has a color index value from 0
	//    (lightest green) to 3 (darkest green). Example with random values:
	//
	//    char ImageTile[64]: [1 0 3 0 2 1 0 3
	//                                  0 1 3 2 1 0 2 0
	//                                  0 1 2 0 3 1 1 2
	//                                  0 3 0 2 3 1 0 2
	//                                  3 1 0 3 0 2 3 0
	//                                  0 2 1 3 0 3 2 1
	//                                  0 3 3 2 1 0 1 2
	//                                  3 0 2 3 1 0 2 2]
	//
	// 2- DataTile, which also represents a tile, in this case using 8 rows of
	//    uint16 (16 bits per row, each pixel is 2 bits, so 8 pixels per row).
	//    Right now all values are 0, waiting to be filled with the values of
	//    the ImageTile:
	//
	//    uint16_t row [8]: [00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000
	//                       00000000 00000000]
	//
	// For every pixel in ImageTile, we have to get those significant last 2
	// bits of the char containing the color value, and place them in
	// the right position of their row in DataTile. But the Game Boy uses a very
	// specific format. Instead of storing those 2 bits consecutively, the low
	// bit (the rightmost one) is stored in the first byte of the tile, and the
	// high bit (the leftmost one) is stored in the second byte. For example,
	// after processing the first pixel above the result would be:
	//
	// uint16_t row [8]: [10000000 00000000 <=> [ 1] - Decimal
	//                    00000000 00000000     [01] - Binary
	//                    00000000 00000000       1  - Low
	//                    00000000 00000000      0   - High
	//                    00000000 00000000
	//                    00000000 00000000
	//                    00000000 00000000
	//                    00000000 00000000]
	//
	// After processing the whole first row (8 pixels) the result would be:
	//
	// uint16_t row [8]: [10100101 00101001 <=> [ 1  0  3  0  2  1  0  3] - Decimal
	//                    00000000 00000000     [01 00 11 00 10 01 00 11] - Binary
	//                    00000000 00000000       1  0  1  0  0  1  0  1  - Low
	//                    00000000 00000000      0  0  1  0  1  0  0  1   - High
	//                    00000000 00000000
	//                    00000000 00000000
	//                    00000000 00000000
	//                    00000000 00000000]
	//
	// When all 64 pixels are processed, this tile is done.
	
	char UCbitPair = 1; /**< Current bit pair to write (we go left to right). */
	char UCtileRow = 0; /**< Current row being written in this tile. */
	
	for (char pixel = 0; pixel < 64; pixel++)
	{
		// Get the individual bits of the color value, low (right) and high
		// (left). Important: the variables must be 16-bit.
		uint16_t UClowBit = (* POimageTile)[pixel] & 0x1;         // Mask against 00000001.
		uint16_t UChighBit = ((* POimageTile)[pixel] & 0x2) >> 1; // Mask against 00000010.
		
		// Shift bits to the left and store them.
		POdataTile->row[UCtileRow] = (POdataTile->row[UCtileRow] | (UClowBit << (16 - UCbitPair)));
		POdataTile->row[UCtileRow] = (POdataTile->row[UCtileRow] | (UChighBit << (8 - UCbitPair)));
		
		// Row finished? Switch to the next.
		if (UCbitPair == 8)
		{
			UCbitPair = 1;
			UCtileRow++;
		}
		else
			UCbitPair++;
	}
}

static void
image2gb_check_duplicates(void)
{
	unsigned int UIduplicateCount     = 0; /**< Number of duplicate tiles that were found. */
	unsigned int UIpreviousDuplicates = 0; /**< Auxiliary variable for storing how many duplicates before the current tile. */
	
	gboolean BisDuplicate; /**< Auxiliary variable for checking if a tile is duplicate. */
	
	// Initialize count to the maximum possible number of tiles.
	UItileCount = (UItileWidth * UItileHeight);
	
	// Initialize tilemap.
	for (unsigned int tile = 0; tile < UItileCount; tile++)
		AtileMap[tile] = tile;
		
	// Right now the image data has as many different tiles as the full original
	// image, and the tilemap is just a count from 0 to N-1 (N being the number
	// of tiles). We have to check if any of the tiles is a duplicate, because
	// in that case we could save video memory by removing it. The algorithm is
	// simple: we traverse the data tiles, and for each one, we check if any of
	// the next ones are identical. If true, we mark those as duplicates, then
	// in the tilemap we replace them. For example, if we are checking tile 37,
	// and we find that tile 61 is a copy, we would mark tile 61 as duplicate
	// and in the tilemap replace "61" for "37". But, because in the final data
	// duplicate tiles will be removed, we have to substract the current number
	// of duplicates that have been found up to that moment. So, if there are 11
	// duplicates before it so far, the correct tile value would be 26, not 37,
	// because in the final tileset those 11 tiles before it will be removed.
	
	for (unsigned int tile = 0; tile < UItileCount; tile++)
	{
		// Do not check tiles already marked as duplicate.
		if (AdataTiles[tile].duplicate == TRUE)
		{
			UIpreviousDuplicates++;
			
			continue;
		}
		
		// Substract the number of duplicated tiles that exist up to this tile's
		// position, to get the correct position it will be in when we output
		// the final data array.
		AtileMap[tile] = (tile - UIpreviousDuplicates);
		
		// Do not check previous tiles, only check forward.
		for (unsigned int checktile = (tile + 1); checktile < UItileCount; checktile++)
		{
			BisDuplicate = FALSE;
			
			// To see if they are equal, we have to compare row per row.
			for (char row = 0; row < 8; row++)
			{
				if (AdataTiles[tile].row[row] != AdataTiles[checktile].row[row])
					break;
					
				if (row == (8 - 1))
					BisDuplicate = TRUE;
			}
			
			if (BisDuplicate)
			{
				AdataTiles[checktile].duplicate = TRUE;
				UIduplicateCount++;
				
				// Replace its value in the tilemap with the one of the tile it
				// is a duplicate of (its value was already corrected above).
				AtileMap[checktile] = AtileMap[tile];
			}
		}
	}
	
	UItileCount = UItileCount - UIduplicateCount;
}

static GimpPDBStatusType
image2gb_write_files(GimpRunMode ErunMode)
{
	FILE* POfileOut; /**< File handler for writing output. */
	
	char SfileName[PATH_MAX] = {0}; /**< Auxiliary string for composing the full file names. */
	
	char SnameLowercase[IMAGE2GB_ASSET_NAME_MAX + 1] = {0}; /**< Asset name, all lowercase. */
	char SnameUppercase[IMAGE2GB_ASSET_NAME_MAX + 1] = {0}; /**< Asset name, all UPPERCASE. */
	
	char* SbankSupport; /**< Output that will only be written if the asset is banked. */
	
	char* SformattedMessage; /**< String used to print error messages. */
	
	int Ierror; /**< Stores the last error code. */
	
	// When writing the final .c source file, the values will be in hexadecimal.
	// The Game Boy expects the asset data as chars (8-bit). Each tile
	// row is 16-bit, so we have to take half and half and convert them to hex.
	// 2 hex digits equal 8 bits (1 byte), so using the same example above:
	//
	//  [1 0 3 0 2 1 0 3] <=> [10100101 00101001] <=> [0xA5, 0x29] <=> 8 pixels
	//
	// Repeat this for the remaining 7 rows and you have a full tile, with 16
	// bytes. Repeat for all tiles and you have the final image. Tiles marked as
	// duplicate are ignored and not written. The tilemap is written as it is,
	// also in hexadecimal.
	
	// Get the all upper and lowercase versions of the asset name.
	for (unsigned int c = 0; c < strlen(SparamAssetName); c++)
	{
		SnameLowercase[c] = tolower(SparamAssetName[c]);
		SnameUppercase[c] = toupper(SparamAssetName[c]);
	}
	
	// First, write the .h header file.
	snprintf(SfileName, PATH_MAX, "%s/%s.h", SparamFolder, SnameLowercase);
	POfileOut = fopen(SfileName, "w");
	
	if (POfileOut == NULL)
	{
		// Save error code before calling another function (may be overwritten).
		Ierror = errno;
		
		SformattedMessage = g_strdup_printf("Could not open file %s, error code %d (%s).\n",
		                                    SfileName, Ierror, strerror(Ierror));
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return GIMP_PDB_EXECUTION_ERROR;
	}
	
	// This part of the header only gets written if the asset is in a bank other
	// than 0. It adds the necessary GBDK-2020 header and the BANKREFs.
	SbankSupport = g_strdup_printf("#include <gb/gb.h>\n\nBANKREF_EXTERN(BACKGROUND_%s)", SnameUppercase);
	
	// Check "source_strings.h" to see exactly what is getting printed here.
	fprintf(POfileOut, SsourceStringH,
	        SnameLowercase, SparamAssetName,
	        UItileCount, (UItileWidth * UItileHeight), UItileWidth, UItileHeight,
	        (UItileWidth * IMAGE2GB_TILE_SIZE), (UItileHeight * IMAGE2GB_TILE_SIZE),
	        UIparamBank,
	        (UIparamBank == 0) ? "#include <stdint.h>" : (char*) SbankSupport,
	        SnameUppercase, UItileCount, SnameUppercase, UItileWidth, SnameUppercase, UItileHeight,
	        SparamAssetName, SparamAssetName, SparamAssetName, SparamAssetName);
	        
	g_free(SbankSupport);
	
	if (fclose(POfileOut) != 0)
	{
		// Save error code before calling another function (may be overwritten).
		Ierror = errno;
		
		SformattedMessage = g_strdup_printf("While trying to close file %s, got error code %d (%s).\n",
		                                    SfileName, Ierror, strerror(Ierror));
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return GIMP_PDB_EXECUTION_ERROR;
	}
	
	// Now, write the .c source file.
	memset(SfileName, 0, sizeof(SfileName));
	snprintf(SfileName, PATH_MAX, "%s/%s.c", SparamFolder, SnameLowercase);
	POfileOut = fopen(SfileName, "w");
	
	if (POfileOut == NULL)
	{
		// Save error code before calling another function (may be overwritten).
		Ierror = errno;
		
		SformattedMessage = g_strdup_printf("Could not open file %s, error code %d (%s).\n",
		                                    SfileName, Ierror, strerror(Ierror));
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return GIMP_PDB_EXECUTION_ERROR;
	}
	
	// This part of the source only gets written if the asset is in a bank other
	// than 0. It adds the necessary GBDK-2020 header and the BANKREFs.
	SbankSupport = g_strdup_printf("#include <gb/gb.h>\n\nBANKREF(BACKGROUND_%s)", SnameUppercase);
	
	// Check "source_strings.h" to see exactly what is getting printed here.
	fprintf(POfileOut, SsourceStringC1,
	        SnameLowercase, SparamAssetName,
	        UItileCount, (UItileWidth * UItileHeight), UItileWidth, UItileHeight,
	        (UItileWidth * IMAGE2GB_TILE_SIZE), (UItileHeight * IMAGE2GB_TILE_SIZE),
	        UIparamBank,
	        SnameLowercase,
	        (UIparamBank == 0) ? "#include <stdint.h>" : (char*) SbankSupport,
	        SparamAssetName);
	        
	g_free(SbankSupport);
	
	image2gb_write_tile_data(POfileOut);
	
	fprintf(POfileOut, SsourceStringC2, SparamAssetName);
	
	image2gb_write_tilemap(POfileOut);
	
	fprintf(POfileOut, "\n};");
	
	if (fclose(POfileOut) != 0)
	{
		// Save error code before calling another function (may be overwritten).
		Ierror = errno;
		
		SformattedMessage = g_strdup_printf("While trying to close file %s, got error code %d (%s).\n",
		                                    SfileName, Ierror, strerror(Ierror));
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return GIMP_PDB_EXECUTION_ERROR;
	}
	
	return GIMP_PDB_SUCCESS;
}

static void
image2gb_write_tile_data(FILE* POfileOut)
{
	unsigned int UIprintCount = 0; /**< Auxiliary variable to keep track of how many tiles we have written. */
	
	// Print one tile per line.
	for (unsigned int tile = 0; tile < (UItileWidth * UItileHeight); tile++)
	{
		// Ignore duplicate tiles.
		if (AdataTiles[tile].duplicate == TRUE)
			continue;
			
		UIprintCount++;
		
		// We do not check for success, we take for granted that we can write.
		fprintf(POfileOut, "\t");
		
		// There are 8 rows, each row is 2 hex numbers, so 16 per line in total.
		for (char row = 0; row < IMAGE2GB_TILE_SIZE; row++)
		{
			// Shift right by 8 bits to write only the first half.
			fprintf(POfileOut, "0x%02X, ", ((AdataTiles[tile].row[row]) >> 8));
			// Mask against 00000000 11111111 to write only the second half.
			fprintf(POfileOut, "0x%02X", ((AdataTiles[tile].row[row]) & 0xFF));
			
			// Do not write a comma after the last char of this tile.
			if (row != (IMAGE2GB_TILE_SIZE - 1))
				fprintf(POfileOut, ", ");
		}
		
		// Do not write a comma after the last tile.
		if (UIprintCount < UItileCount)
			fprintf(POfileOut, ",\n");
		else
			fprintf(POfileOut, "\n");
	}
}

static void
image2gb_write_tilemap(FILE* POfileOut)
{
	// We do not check for success, we take for granted that we can write.
	fprintf(POfileOut, "\t");
	
	for (unsigned int tile = 0; tile < (UItileWidth * UItileHeight); tile++)
	{
		fprintf(POfileOut, "0x%02X", AtileMap[tile]);
		
		// If this is not the last tile of the map, print a separator.
		if (tile != ((UItileWidth * UItileHeight) - 1))
		{
			// Print lines of "width" tiles maximum (so the output code has as
			// many rows and columns as the image, for easier debugging).
			if (((tile + 1) % UItileWidth) == 0)
				fprintf(POfileOut, ",\n\t");
			else
				fprintf(POfileOut, ", ");
		}
	}
}
