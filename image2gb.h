/**
 * @file  image2gb.h
 * @brief GIMP plugin to export an image to Game Boy data (C code, for use with GBDK-2020) - header.
 */

#pragma once

// Ignore warnings in external libraries (GIMP, GTK...).
#pragma GCC system_header
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <ctype.h>

// CONSTANTS ///////////////////////////////////////////////////////////////////

#define IMAGE2GB_BINARY_NAME    "image2gb"        /**< Name of the output binary. */
#define IMAGE2GB_PROCEDURE_MENU "Image2GB-menu"   /**< Name of the procedure registered as menu entry. */
#define IMAGE2GB_PROCEDURE_SAVE "Image2GB-export" /**< Name of the procedure registered as save handler. */

#define IMAGE2GB_DESCRIPTION_SHORT "Export image to Game Boy data"
#define IMAGE2GB_DESCRIPTION_LONG  "Exports an indexed 4-color image to Game Boy data (C code, for use with GBDK-2020)."
#define IMAGE2GB_AUTHOR            "DaSalba"
#define IMAGE2GB_COPYRIGHT         "Copyright (c) 2020-2025 DaSalba"
#define IMAGE2GB_DATE              "2025"
#define IMAGE2GB_MENU_NAME         "Game Boy (GBDK-2020)" /**< Entry that will appear in the menus and "Export as" dialog. */
#define IMAGE2GB_IMAGE_TYPES       "INDEXED"              /**< What type of images the plugin supports (RGB, GRAY, INDEXED...). */
#define IMAGE2GB_MENU_PATH         "<Image>/Tools"        /**< Category, and menu path the plugin will appear in. */

#define IMAGE2GB_ASSOCIATED_MIME_TYPE "text/plain" /**< MIME file type that will be associated with this plugin. */
#define IMAGE2GB_ASSOCIATED_EXTENSION "gbdk"       /**< File extension that will be associated with this plugin. */

#define IMAGE2GB_ASSET_NAME_MAX_LENGTH 32U /**< Max characters of the asset name used for the C variable identifier. */

#define IMAGE2GB_PARASITE "gbdk-2020-export-options" /**< Cookie to store export parameters between invocations (persistent data). */

// DEFINITIONS /////////////////////////////////////////////////////////////////

/** Object that stores this plugin's export parameters.
 */
typedef struct PluginExportOptions
{
	gchar name[IMAGE2GB_ASSET_NAME_MAX_LENGTH]; /**< Base name of the image asset to export. */
	gchar folder[PATH_MAX];                     /**< Full path of the directory to save to. */
	gint bank;                                  /**< ROM bank to store the image data in. */
} PluginExportOptions;

// FUNCTIONS ///////////////////////////////////////////////////////////////////

/** Returns information about this plugin whenever it is loaded or changes.
 */
static void
image2gb_query(void);

/** Called when the plugin is run, it does the job.
 */
static void
image2gb_run(const gchar* Sname, gint InumParams, const GimpParam* Gparams, gint* InumReturnVals, GimpParam** GreturnVals);

/** Checks the validity of the image for being exported to Game Boy. Returns
 *  TRUE if it is valid, FALSE otherwise.
 */
static gboolean
image2gb_check_image(gint32 IimageID);

/** Opens a dialog window to let the user set the export parameters. Returns
 *  the program status.
 */
static GimpPDBStatusType
image2gb_show_dialog(void);

/** Callback function for the GTK dialog window ("Export" or "Cancel" clicked).
 */
static void
image2gb_dialog_response(GtkWidget* Wwidget, gint IresponseID, gpointer Pdata);

/** Tries to load existing export parameters from the parasite. Returns TRUE if
 *  success, FALSE otherwise.
 */
static gboolean
image2gb_load_parameters(gint32 IimageID);

/** Saves the current export parameters using a parasite.
 */
static void
image2gb_save_parameters(gint32 IimageID);
