/**
 * @file image_export.h
 * @brief Functionality for exporting a GIMP indexed image to Game Boy data. (header+implementation)
 */

#ifndef IMAGE2GB_IMAGE_EXPORT_H_INCLUDED
#define IMAGE2GB_IMAGE_EXPORT_H_INCLUDED

#include "image2gb.h" // For PluginExportOptions.
#include "source_strings.h"

// Ignore warnings in external libraries (GIMP, GTK...).
#pragma GCC system_header
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

// DEFINITIONS /////////////////////////////////////////////////////////////////

#define IMAGE2GB_TILE_SIZE 8 /**< Size of a tile, in pixels (X or Y). */

#define IMAGE2GB_IMAGE_SIZE_MIN IMAGE2GB_TILE_SIZE /**< Minimum acceptable image size, in pixels (X or Y). */
#define IMAGE2GB_IMAGE_SIZE_MAX 256                /**< Maximum acceptable image size, in pixels (X or Y). */

#define IMAGE2GB_IMAGE_TILES_VRAM_LIMIT 256 /**< How many unique tiles will fit in GB's memory at a time. */

#define IMAGE2GB_TILEMAP_TILES_PER_LINE 20 /**< In the final .c, how many chars of the tilemap to write per line. */

/** Object that represents a tile on the GIMP image: a 8x8 pixel square.
 */
typedef guchar ImageTile[IMAGE2GB_TILE_SIZE * IMAGE2GB_TILE_SIZE];

/** Object that represents a Game Boy tile: a 8x8 square with 4-color (2 bit)
 *  pixels. Hence, a tile has 64 * 2 = 128 bits of data, in 8 rows of 16 bits
 *  each.
 */
typedef struct DataTile
{
	uint16_t row[IMAGE2GB_TILE_SIZE]; /**< Array that stores the 8 pixel rows of this tile. */
	gboolean duplicate; /**< Flag for marking this tile as a duplicate of another. */
} DataTile;

// VARIABLES ///////////////////////////////////////////////////////////////////

/** Array that stores all tiles of the image, in Game Boy data format.
 */
DataTile ArrayDataTiles[(IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)
                        * (IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)] = {0};

/** Array that stores the tilemap of the image, in Game Boy data format.
 */
guint ArrayTileMap[(IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)
                   * (IMAGE2GB_IMAGE_SIZE_MAX / IMAGE2GB_TILE_SIZE)] = {0};

guint UItileWidth = 0; /**< Width of the asset in Game Boy tiles. */

guint UItileHeight = 0; /**< Height of the asset in Game Boy tiles. */

guint UItileCount = 0; /**< Total number of tiles the asset has. */

// FUNCTIONS ///////////////////////////////////////////////////////////////////

/** Tries to export the image. Returns the program status.
 */
static GimpPDBStatusType
image2gb_export_image(gint32 IimageID, gint32 IdrawableID, PluginExportOptions* PexportOptions);

/** Reads the GIMP image and populates the tile array accordingly.
 */
static void
image2gb_read_image_tiles(gint32 IimageID, gint32 IdrawableID);

/** Parses a tile from the GIMP image and stores it in the given DataTile.
 */
static void
image2gb_read_tile(ImageTile* PimageTile, DataTile* PdataTile);

/** Checks all tiles and finds the duplicates, removing them from the tilemap.
 */
static void
image2gb_check_duplicates(void);

/** Writes the output .h header and .c source files containing the image asset.
 *  Returns the program status.
 */
static GimpPDBStatusType
image2gb_write_files(PluginExportOptions* PexportOptions);

/** Writes the asset tile data to the given file in the format expected by GBDK.
 */
static void
image2gb_write_tile_data(FILE* FileOut);

/** Writes the asset tilemap to the given file, in the format expected by GBDK.
 */
static void
image2gb_write_tilemap(FILE* FileOut);

////////////////////////////////////////////////////////////////////////////////

static GimpPDBStatusType
image2gb_export_image(gint32 IimageID, gint32 IdrawableID, PluginExportOptions* PexportOptions)
{
	GimpPDBStatusType GreturnStatus = GIMP_PDB_SUCCESS; /**< Return value. */
	
	// Compute image statistics.
	UItileWidth = (gimp_image_width(IimageID) / IMAGE2GB_TILE_SIZE);
	UItileHeight = (gimp_image_height(IimageID) / IMAGE2GB_TILE_SIZE);
	
	image2gb_read_image_tiles(IimageID, IdrawableID);
	
	image2gb_check_duplicates();
	
	// Give a warning if the image will not fit in the Game Boy's VRAM.
	if (UItileCount > IMAGE2GB_IMAGE_TILES_VRAM_LIMIT)
		g_message("WARNING: this image has %u unique tiles. The Game Boy video memory can only fit " \
		          "up to %d at the same time (384 using a hack). It will probably give errors.\n",
		          UItileCount, IMAGE2GB_IMAGE_TILES_VRAM_LIMIT);
		          
	image2gb_write_files(PexportOptions);
	
	return GreturnStatus;
}

static void
image2gb_read_image_tiles(gint32 IimageID, gint32 IdrawableID)
{
	GimpDrawable* Gdrawable = gimp_drawable_get(IdrawableID); /**< GIMP drawable object that represents the image. */
	GimpPixelRgn Gregion = {0}; /**< GIMP pixel region for reading the image. */
	ImageTile imageTile = {0}; /**< Array that stores the GIMP pixels of a tile (8x8). */
	
	// Initialize the pixel region for later use.
	gimp_pixel_rgn_init(&Gregion, Gdrawable,
	                    0, 0,
	                    gimp_image_width(IimageID), gimp_image_height(IimageID),
	                    FALSE, FALSE);
	                    
	// Loop through all tiles of the GIMP image.
	for (unsigned int row = 0; row < UItileHeight; row++)
	{
		for (unsigned int col = 0; col < UItileWidth; col++)
		{
			// Get the 64 pixels for this tile.
			gimp_pixel_rgn_get_rect(&Gregion, imageTile,
			                        IMAGE2GB_TILE_SIZE * col, IMAGE2GB_TILE_SIZE * row,
			                        IMAGE2GB_TILE_SIZE, IMAGE2GB_TILE_SIZE);
			                        
			// Call this other function which will parse and store that tile.
			// "array + n" gets the address of the nth element of the array.
			image2gb_read_tile(imageTile, ArrayDataTiles + ((row * UItileWidth) + col));
		}
	}
}

static void
image2gb_read_tile(ImageTile* PimageTile, DataTile* PdataTile)
{
	// Visual explanation: right now we are processing a single tile, which is a
	// square 8x8 pixel area of the image, 64 pixels in total. We have 2
	// variable types:
	//
	// 1- ImageTile, filled with 64 guchar that contain the values of every
	// pixel of this tile. Every pixel has a color index value from 0 (lightest
	// green) to 3 (darkest green). Example with random values:
	//
	//  guchar ImageTile[64]: [1 0 3 0 2 1 0 3
	//                         0 1 3 2 1 0 2 0
	//                         0 1 2 0 3 1 1 2
	//                         0 3 0 2 3 1 0 2
	//                         3 1 0 3 0 2 3 0
	//                         0 2 1 3 0 3 2 1
	//                         0 3 3 2 1 0 1 2
	//                         3 0 2 3 1 0 2 2]
	//
	// 2- DataTile, which also represents a tile, in this case using 8
	// rows of uint16 (16 bits per row, each pixel is 2 bits, so 8 pixels per
	// row). Right now all values are 0, waiting to be filled with the values of
	// ImageTile:
	//
	//  uint16_t row [8]: [00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000]
	//
	// For every pixel in ImageTile, we have to get those significant last 2
	// bits of the guchar containing the color value, and place them in the
	// right position of their row in DataTile. But the Game Boy uses a very
	// specific format. Instead of storing those 2 bits consecutively, the low
	// bit (the rightmost one) is stored in the first byte of the tile, and the
	// high bit (the leftmost one) is stored in the second byte. For example,
	// after processing the first pixel above the result would be:
	//
	//  uint16_t row [8]: [10000000 00000000 <=> [ 1]
	//                     00000000 00000000     [01]
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000]
	//
	// After processing the whole first row (8 pixels) the result would be:
	//
	//  uint16_t row [8]: [10100101 00101001 <=> [ 1  0  3  0  2  1  0  3]
	//                     00000000 00000000     [01 00 11 00 10 01 00 11]
	//                     00000000 00000000       1  0  1  0  0  1  0  1 - Low
	//                     00000000 00000000      0  0  1  0  1  0  0  1  - High
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000
	//                     00000000 00000000]
	//
	// When all 64 pixels are processed, this tile is done.
	
	guchar UCbitPair = 1; /**< Current bit pair to write (we go left to right). */
	guchar UCtileRow = 0; /**< Current row being written in this tile. */
	
	for (guchar pixel = 0; pixel < 64; pixel++)
	{
		// Get the individual bits of the color value, low (right) and high
		// (left). Important, the variables must be 16-bit.
		uint16_t UClowBit = (*PimageTile)[pixel] & 0x1;  // Mask against 00000001.
		uint16_t UChighBit = ((*PimageTile)[pixel] & 0x2) >> 1; // Mask against 00000010.
		
		// Shift bits to the left and store them.
		PdataTile->row[UCtileRow] = (PdataTile->row[UCtileRow] | (UClowBit << (16 - UCbitPair)));
		PdataTile->row[UCtileRow] = (PdataTile->row[UCtileRow] | (UChighBit << (8 - UCbitPair)));
		
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
	guint UIduplicateCount = 0; /**< Number of duplicate tiles that were found. */
	gboolean BisDuplicate = FALSE; /**< Auxiliary variable for checking if a tile is duplicate. */
	guint UIpreviousDuplicates = 0; /**< Auxiliary variable for storing how many duplicates before the current tile. */
	
	// Initialize count to the maximum possible number of tiles.
	UItileCount = (UItileWidth * UItileHeight);
	
	// Initialize tilemap.
	for (guint tile = 0; tile < UItileCount; tile++)
		ArrayTileMap[tile] = tile;
		
	// Right now the image data has as many different tiles as the full original
	// image, and the tilemap is just a count from 0 to N-1 (N being the number
	// of tiles). We have to check if any of the tiles are duplicated, because
	// in that case we could save video memory by removing it. The algorithm is
	// simple: we traverse the data tiles, and for each one, we check if any of
	// the next ones are identical. If true, we mark those as duplicates, then
	// in the tilemap we replace them. For example, if we are checking tile 37,
	// and we find that tile 61 is a copy, we would mark tile 61 as duplicate
	// and in the tilemap replace "61" for "37". But, because in the final data
	// duplicate tiles will be removed, we have to substract the current number
	// of duplicates that have been found up to that moment. So, if there are 11
	// duplicates before it so far, the correct tile value would be 26, not 37,
	// because in the final data, those 11 tiles before it will be suppressed.
	
	for (guint tile = 0; tile < UItileCount; tile++)
	{
		// Do not check tiles already marked as duplicate.
		if (ArrayDataTiles[tile].duplicate == TRUE)
		{
			UIpreviousDuplicates++;
			
			continue;
		}
		
		// Substract the number of duplicated tiles that exist up to this tile's
		// position, to get the correct position it will be in when we output
		// the final data array.
		ArrayTileMap[tile] = (tile - UIpreviousDuplicates);
		
		// Do not check previous tiles, only check forward.
		for (guint checktile = (tile + 1); checktile < UItileCount; checktile++)
		{
			BisDuplicate = FALSE;
			
			// To see if they are equal, we have to compare row per row.
			for (guchar row = 0; row < 8; row++)
			{
				if (ArrayDataTiles[tile].row[row] != ArrayDataTiles[checktile].row[row])
					break;
					
				if (row == (8 - 1))
					BisDuplicate = TRUE;
			}
			
			if (BisDuplicate)
			{
				ArrayDataTiles[checktile].duplicate = TRUE;
				UIduplicateCount++;
				
				// Replace its value in the tilemap with the one of the tile it
				// is a duplicate of (its value was already corrected above).
				ArrayTileMap[checktile] = ArrayTileMap[tile];
			}
		}
	}
	
	UItileCount = UItileCount - UIduplicateCount;
}

static GimpPDBStatusType
image2gb_write_files(PluginExportOptions* PexportOptions)
{
	GimpPDBStatusType GreturnStatus = GIMP_PDB_SUCCESS; /**< Return value. */
	FILE* FileOut = NULL; /**< File handler for writing output. */
	gchar SfileName[PATH_MAX] = {0}; /**< Auxiliary string for composing the full file names. */
	gchar SNameLowercase[IMAGE2GB_ASSET_NAME_MAX_LENGTH] = {0}; /**< Asset name, all lowercase. */
	gchar SNameUppercase[IMAGE2GB_ASSET_NAME_MAX_LENGTH] = {0}; /**< Asset name, all UPPERCASE. */
	
	// When writing the final .c source file, the values will be in hexadecimal.
	// The Game Boy expects the asset data as unsigned chars (8-bit). Each tile
	// row is 16-bit, so we have to take half and half and convert them to hex.
	// 2 hex digits equal 8 bits, so using the same example above:
	//
	//  [1 0 3 0 2 1 0 3] <=> [10100101 00101001] <=> [0xA5, 0x29] <=> 8 pixels
	//
	// Repeat this for the remaining 7 rows and you have a full tile, with 16
	// hex values. Repeat for all tiles and you have the final image. Tiles
	// marked as duplicate are ignored and not written. The tilemap is written
	// as it is, also in hexadecimal.
	
	// Get the all upper and lowercase version of the asset name.
	for (guint c = 0; c < strlen(PexportOptions->name); c++)
		SNameLowercase[c] = tolower(PexportOptions->name[c]);
		
	for (guint c = 0; c < strlen(PexportOptions->name); c++)
		SNameUppercase[c] = toupper(PexportOptions->name[c]);
		
	// First, write the .h header.
	sprintf(SfileName, "%s/%s.h", PexportOptions->folder, SNameLowercase);
	FileOut = fopen(SfileName, "w");
	
	if (FileOut == NULL)
	{
		// Save error code before calling another function (may be overwritten).
		gint Ierror = errno;
		g_message("Could not open file %s, error code %d (%s).\n", SfileName, Ierror, strerror(Ierror));
		
		GreturnStatus = GIMP_PDB_EXECUTION_ERROR;
		
		return GreturnStatus;
	}
	
	// Check "source_strings.h" to see what we're printing here.
	fprintf(FileOut, IMAGE2GB_SOURCE_STRING_H,
	        SNameLowercase, ".h", PexportOptions->name,
	        UItileCount, (UItileWidth * UItileHeight), UItileWidth, UItileHeight,
	        (UItileWidth * IMAGE2GB_TILE_SIZE), (UItileHeight * IMAGE2GB_TILE_SIZE),
	        SNameUppercase, SNameUppercase,
	        (PexportOptions->bank == 0) ? "//" : "", SNameUppercase, PexportOptions->bank, // If bank = 0, line is commented out.
	        SNameUppercase, UItileCount, SNameUppercase, UItileWidth, SNameUppercase, UItileHeight,
	        PexportOptions->name, PexportOptions->name, PexportOptions->name, PexportOptions->name,
	        SNameUppercase);
	        
	if (fclose(FileOut) != 0)
	{
		// Save error code before calling another function (may be overwritten).
		gint Ierror = errno;
		g_message("While trying to close file %s, got error code %d (%s).\n", SfileName, Ierror, strerror(Ierror));
		
		GreturnStatus = GIMP_PDB_EXECUTION_ERROR;
		
		return GreturnStatus;
	}
	
	// Now, write the .c source.
	memset(SfileName, 0, sizeof(SfileName));
	sprintf(SfileName, "%s/%s.c", PexportOptions->folder, SNameLowercase);
	FileOut = fopen(SfileName, "w");
	
	if (FileOut == NULL)
	{
		// Save error code before calling another function (may be overwritten).
		gint Ierror = errno;
		g_message("Could not open file %s, error code %d (%s).\n", SfileName, Ierror, strerror(Ierror));
		
		GreturnStatus = GIMP_PDB_EXECUTION_ERROR;
		
		return GreturnStatus;
	}
	
	// Check "source_strings.h" to see what we're printing here.
	fprintf(FileOut, IMAGE2GB_SOURCE_STRING_C_1,
	        SNameLowercase, ".c", PexportOptions->name,
	        UItileCount, (UItileWidth * UItileHeight), UItileWidth, UItileHeight,
	        (UItileWidth * IMAGE2GB_TILE_SIZE), (UItileHeight * IMAGE2GB_TILE_SIZE),
	        SNameLowercase, ".h", PexportOptions->name);
	        
	image2gb_write_tile_data(FileOut);
	
	fprintf(FileOut, IMAGE2GB_SOURCE_STRING_C_2, PexportOptions->name);
	
	image2gb_write_tilemap(FileOut);
	
	fprintf(FileOut, "\n};");
	
	if (fclose(FileOut) != 0)
	{
		// Save error code before calling another function (may be overwritten).
		gint Ierror = errno;
		g_message("While trying to close file %s, got error code %d (%s).\n", SfileName, Ierror, strerror(Ierror));
		
		GreturnStatus = GIMP_PDB_EXECUTION_ERROR;
		
		return GreturnStatus;
	}
	
	return GreturnStatus;
}

static void
image2gb_write_tile_data(FILE* FileOut)
{
	guint UIprintCount = 0; /**< Auxiliary variable to keep track of how many tiles we have written. */
	
	// Print one tile per line.
	for (guint tile = 0; tile < (UItileWidth * UItileHeight); tile++)
	{
		// Ignore duplicate tiles.
		if (ArrayDataTiles[tile].duplicate == TRUE)
			continue;
			
		UIprintCount++;
		
		// We do not check for success, we take for granted we can write OK.
		fprintf(FileOut, "\t");
		
		// There are 8 rows, each row is 2 hex numbers, so 16 per line in total.
		for (guchar row = 0; row < IMAGE2GB_TILE_SIZE; row++)
		{
			// Shift right by 8 bits to write only the first half.
			fprintf(FileOut, "0x%02X, ", ((ArrayDataTiles[tile].row[row]) >> 8));
			// Mask against 00000000 11111111 to write only the second half.
			fprintf(FileOut, "0x%02X", ((ArrayDataTiles[tile].row[row]) & 0xFF));
			
			// Do not write a comma after the last char of this tile.
			if (row != (IMAGE2GB_TILE_SIZE - 1))
				fprintf(FileOut, ", ");
		}
		
		// Do not write a comma after the last tile.
		if (UIprintCount < UItileCount)
			fprintf(FileOut, ",\n");
		else
			fprintf(FileOut, "\n");
	}
}

static void
image2gb_write_tilemap(FILE* FileOut)
{
	// We do not check for success, we take for granted we can write OK.
	fprintf(FileOut, "\t");
	
	for (guint tile = 0; tile < (UItileWidth * UItileHeight); tile++)
	{
		fprintf(FileOut, "0x%02X", ArrayTileMap[tile]);
		
		// If this is not the last tile of the map, print a separator.
		if (tile != ((UItileWidth * UItileHeight) - 1))
		{
			// Lines of 20 tiles maximum (20 tiles = 160 pixels = fullscreen width).
			if (((tile + 1) % IMAGE2GB_TILEMAP_TILES_PER_LINE) == 0)
				fprintf(FileOut, ",\n\t");
			else
				fprintf(FileOut, ", ");
		}
	}
}

#endif // IMAGE2GB_IMAGE_EXPORT_H_INCLUDED
