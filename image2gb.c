/**
 * @file  image2gb.c
 * @brief GIMP plugin to export an image to Game Boy data (C code, for use with GBDK-2020) - implementation.
 */

#include "image2gb.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <string.h>

#include "image_export.h"

// CONSTANTS ///////////////////////////////////////////////////////////////////

#define IMAGE2GB_BINARY_NAME "image2gb" /**< Name of the output binary. */

#define IMAGE2GB_PROCEDURE_MENU   "plug-in-image2gb-menu"   /**< Name of the procedure registered as menu entry. */
#define IMAGE2GB_PROCEDURE_EXPORT "plug-in-image2gb-export" /**< Name of the procedure registered as file export handler. */

#define IMAGE2GB_MENU_PATH  "<Image>/Tools"        /**< Window area and menu path the plugin will appear in. */
#define IMAGE2GB_MENU_LABEL "Game Boy (GBDK-2020)" /**< Entry that will appear in the menus and "Export as" dialog. */

#define IMAGE2GB_ASSOCIATED_MIME_TYPE "text/plain" /**< MIME file type that will be associated with this plugin. */
#define IMAGE2GB_ASSOCIATED_EXTENSION "gbdk"       /**< File extension that will be associated with this plugin. */

#define IMAGE2GB_SENSITIVITY GIMP_PROCEDURE_SENSITIVE_DRAWABLE /**< Which content this plugin supports. */

#define IMAGE2GB_PARAM_ASSET_NAME "asset-name" /**< Parameter ID of the asset name. */
#define IMAGE2GB_PARAM_FOLDER     "folder"     /**< Parameter ID of the output path. */
#define IMAGE2GB_PARAM_BANK       "bank"       /**< Parameter ID of the bank number. */

// Plugin documentation and attribution strings.
#define IMAGE2GB_DESCRIPTION_SHORT "Export image to Game Boy data"
#define IMAGE2GB_DESCRIPTION_LONG  "Exports an indexed 4-color image to Game Boy data (C code, for use with GBDK-2020)."
#define IMAGE2GB_AUTHOR            "DaSalba"
#define IMAGE2GB_COPYRIGHT         "Copyright (c) 2020-2025 DaSalba"
#define IMAGE2GB_DATE              "2025"

// VARIABLES ///////////////////////////////////////////////////////////////////

char* SparamAssetName;
char* SparamFolder;

unsigned int UIparamBank;

static GObjectClass* POparentClass; /**< Pointer to the parent class. */

static GList* PLprocedures; /**< Pointer to the list of procedures this plugin provides. */

// FUNCTIONS ///////////////////////////////////////////////////////////////////

/** Checks the suitability of the image for being exported to Game Boy data.
 *
 * \param[in] POimage  Image to check.
 * \param[in] ErunMode The run mode (interactive or not).
 *
 * \return TRUE if the image is valid, FALSE otherwise.
 */
static gboolean
check_image(GimpImage* POimage, GimpRunMode ErunMode);

/** Shows a dialog window where the user can input the plugin parameters.
 *
 * \param[in] POprocedure Pointer to the procedure.
 * \param[in] POconfig    Pointer to the procedure config.
 *
 * \return TRUE if the user clicked "OK", FALSE if he clicked "Cancel".
 */
static gboolean
show_dialog(GimpProcedure* POprocedure, GimpProcedureConfig* POconfig);

/** Retrieves the current value of the plugin parameters (asset name, output
 *  path and bank number) from the configuration, and stores them in global
 *  variables. It also checks their validity.
 *
 * \param[in] POconfig Pointer to the procedure config.
 * \param[in] ErunMode The run mode (interactive or not).
 *
 * \return TRUE if the parameters are valid, FALSE otherwise.
 */
static gboolean
load_parameters(GimpProcedureConfig* POconfig, GimpRunMode ErunMode);

////////////////////////////////////////////////////////////////////////////////

// GObject convenience macro for declaring stuff.
G_DEFINE_TYPE(Image2GB, image2gb, GIMP_TYPE_PLUG_IN)

// GIMP macro that declares a proper main() and initializes everything.
GIMP_MAIN(image2gb_get_type())

static void
image2gb_class_init(Image2GBClass* POclass)
{
	// Override the generic cleanup function.
	GObjectClass* POgenericClass = G_OBJECT_CLASS(POclass);
	POgenericClass->dispose = image2gb_dispose;
	
	// Override the query and create functions.
	GimpPlugInClass* POpluginClass = GIMP_PLUG_IN_CLASS(POclass);
	POpluginClass->query_procedures = image2gb_query_procedures;
	POpluginClass->create_procedure = image2gb_create_procedure;
	
	POparentClass = g_type_class_peek_parent(POclass);
}

static void
image2gb_init(Image2GB* POinstance)
{

}

static GList*
image2gb_query_procedures(GimpPlugIn* POplugin)
{
	// Free memory if there was an existing list.
	if (PLprocedures)
	{
		g_list_free_full(PLprocedures, g_free);
		PLprocedures = NULL;
	}
	
	// This plugin only has 2 procedures.
	PLprocedures = g_list_append(PLprocedures, g_strdup(IMAGE2GB_PROCEDURE_MENU));
	PLprocedures = g_list_append(PLprocedures, g_strdup(IMAGE2GB_PROCEDURE_EXPORT));
	
	return PLprocedures;
}

static GimpProcedure*
image2gb_create_procedure(GimpPlugIn* POplugin,
                          const gchar* Sname)
{
	GimpProcedure* POprocedure = NULL; /**< Pointer to the created GIMP procedure. */
	
	// 1) Menu procedure.
	if (g_strcmp0(Sname, IMAGE2GB_PROCEDURE_MENU) == 0)
	{
		POprocedure = gimp_image_procedure_new(POplugin, Sname,
		                                       GIMP_PDB_PROC_TYPE_PLUGIN,
		                                       image2gb_run_menu, NULL, NULL);
		                                       
		// Register a menu entry in Tools/.
		gimp_procedure_set_menu_label(POprocedure, IMAGE2GB_MENU_LABEL);
		gimp_procedure_add_menu_path(POprocedure, IMAGE2GB_MENU_PATH);
	}
	// 2) File export procedure.
	else if (g_strcmp0(Sname, IMAGE2GB_PROCEDURE_EXPORT) == 0)
	{
		POprocedure = gimp_export_procedure_new(POplugin, Sname,
		                                        GIMP_PDB_PROC_TYPE_PLUGIN,
		                                        FALSE,
		                                        image2gb_run_save, NULL, NULL);
		                                        
		// Name that will appear in the File->Export... menu.
		gimp_procedure_set_menu_label(POprocedure, IMAGE2GB_MENU_LABEL);
		
		gimp_file_procedure_set_format_name(GIMP_FILE_PROCEDURE(POprocedure),
		                                    IMAGE2GB_MENU_LABEL);
		                                    
		// Register MIME file type (probably unnecesary?).
		gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(POprocedure),
		                                   IMAGE2GB_ASSOCIATED_MIME_TYPE);
		                                   
		// Register the file extension this procedure supports. NOTE: the plugin
		// does not really save a file with this extension, it actually saves 2
		// files, a .c source and a .h header. This association is a just a
		// shortcut to make it easier for the user (also, GIMP already has
		// handlers for exporting images to .c and .h extensions).
		gimp_file_procedure_set_extensions(GIMP_FILE_PROCEDURE(POprocedure),
		                                   IMAGE2GB_ASSOCIATED_EXTENSION);
		                                   
		// Set the default file export capabilities for this plugin.
		gimp_export_procedure_set_capabilities(GIMP_EXPORT_PROCEDURE(POprocedure),
		                                       GIMP_EXPORT_CAN_HANDLE_INDEXED,
		                                       NULL, NULL, NULL);
		                                       
		// Set the priority (useful if more than one procedure for the given
		// file format; the one with the lowest priority will be used).
		gimp_file_procedure_set_priority(GIMP_FILE_PROCEDURE(POprocedure), 0);
	}
	
	// Both procedures have the same metadata (plugin info) and parameters.
	if (POprocedure)
	{
		// What type of content this plugin will support for exporting.
		gimp_procedure_set_sensitivity_mask(POprocedure, IMAGE2GB_SENSITIVITY);
		gimp_procedure_set_image_types(POprocedure, "INDEXED");
		
		// Metadata.
		gimp_procedure_set_documentation(POprocedure,
		                                 IMAGE2GB_DESCRIPTION_SHORT,
		                                 IMAGE2GB_DESCRIPTION_LONG,
		                                 NULL);
		gimp_procedure_set_attribution(POprocedure,
		                               IMAGE2GB_AUTHOR,
		                               IMAGE2GB_COPYRIGHT,
		                               IMAGE2GB_DATE);
		                               
		// Parameters.
		gimp_procedure_add_string_argument(POprocedure,
		                                   IMAGE2GB_PARAM_ASSET_NAME,
		                                   "Asset name (C naming rules):",
		                                   "Keep it short and a valid C identifier, it will be the base name for the variables.",
		                                   "",
		                                   G_PARAM_READWRITE);
		gimp_procedure_add_file_argument(POprocedure,
		                                 IMAGE2GB_PARAM_FOLDER,
		                                 "Destination folder:",
		                                 "The .c and .h files will be created in this folder.",
		                                 GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		                                 FALSE,
		                                 NULL,
		                                 G_PARAM_READWRITE);
		gimp_procedure_add_int_aux_argument(POprocedure,
		                                    IMAGE2GB_PARAM_BANK,
		                                    "ROM bank number (optional):",
		                                    "Set it to 0 for using the default bank.",
		                                    0,
		                                    IMAGE2GB_BANK_MAX,
		                                    0,
		                                    G_PARAM_READWRITE);
	}
	
	return POprocedure;
}

GimpValueArray*
image2gb_run_menu(GimpProcedure*       POprocedure,
                  GimpRunMode          ErunMode,
                  GimpImage*           POimage,
                  GimpDrawable**       Adrawables,
                  GimpProcedureConfig* POconfig,
                  gpointer             PrunData)
{
	GimpPDBStatusType EreturnValue; /**< Return value (PDB status) of this procedure. */
	
	int IdrawablesCount; /**< How many drawables are currently selected in GIMP. */
	
	IdrawablesCount = gimp_core_object_array_get_length((GObject**) Adrawables);
	
	// Check that the conditions to run the plugin are met.
	if (IdrawablesCount != 1)
	{
		GError* POerror = NULL;
		
		g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
		            "Procedure '%s' supports exactly one drawable.",
		            IMAGE2GB_PROCEDURE_MENU);
		            
		return gimp_procedure_new_return_values(POprocedure,
		                                        GIMP_PDB_CALLING_ERROR,
		                                        POerror);
	}
	else
	{
		GimpDrawable* PGDdrawable = Adrawables[0];
		
		if (! GIMP_IS_LAYER(PGDdrawable))
		{
			GError* POerror = NULL;
			
			g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
			            "No valid layer selected for '%s'.",
			            IMAGE2GB_PROCEDURE_MENU);
			            
			return gimp_procedure_new_return_values(POprocedure,
			                                        GIMP_PDB_CALLING_ERROR,
			                                        POerror);
		}
	}
	
	// Check if the image meets the requirements to be exported.
	if (! check_image(POimage, ErunMode))
	{
		GError* POerror = NULL;
		
		g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
		            "The image does not meet the requirements. Check size, type and palette.");
		            
		return gimp_procedure_new_return_values(POprocedure,
		                                        GIMP_PDB_CALLING_ERROR,
		                                        POerror);
	}
	
	// This procedure always invokes the GUI to choose the parameters. If the
	// user cancels the dialog, do nothing (this is not an error).
	if (! show_dialog(POprocedure, POconfig))
		return gimp_procedure_new_return_values(POprocedure, GIMP_PDB_CANCEL, NULL);
		
	// Check the validity of the parameters that were given.
	if (! load_parameters(POconfig, ErunMode))
	{
		GError* POerror = NULL;
		
		g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
		            "Incorrect parameters.");
		            
		return gimp_procedure_new_return_values(POprocedure,
		                                        GIMP_PDB_CALLING_ERROR,
		                                        POerror);
	}
	
	// Finally, try to export the image.
	EreturnValue = image2gb_export_image(POimage, ErunMode);
	
	return gimp_procedure_new_return_values(POprocedure, EreturnValue, NULL);
}

GimpValueArray*
image2gb_run_save(GimpProcedure*       POprocedure,
                  GimpRunMode          ErunMode,
                  GimpImage*           POimage,
                  GFile*               POfile,
                  GimpExportOptions*   POoptions,
                  GimpMetadata*        POmetadata,
                  GimpProcedureConfig* POconfig,
                  gpointer             PrunData)
{
	GimpPDBStatusType EreturnValue; /**< Return value (PDB status) of this procedure. */
	
	char* PCdotExtension; /**< Pointer to the location of the file extension dot (NULL if none). */
	
	GFile* POparamFolder = NULL; /**< Virtual file object for setting the "output path" parameter. */
	
	// Check if the image meets the requirements to be exported.
	if (! check_image(POimage, ErunMode))
	{
		GError* POerror = NULL;
		
		g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
		            "The image does not meet the requirements. Check size, type and palette.");
		            
		return gimp_procedure_new_return_values(POprocedure,
		                                        GIMP_PDB_CALLING_ERROR,
		                                        POerror);
	}
	
	// If exporting for the first time, need to show the dialog. First, we
	// recover 2 parameters (asset name and output path) from the destination
	// the user gave at the file export dialog.
	if (ErunMode == GIMP_RUN_INTERACTIVE)
	{
		// Get the full file path from the GFile object, extract the base name
		// (file without path), and remove the extension, if any, so we get the
		// asset name.
		SparamAssetName = g_path_get_basename(g_file_get_path(POfile));
		PCdotExtension = strrchr(SparamAssetName, '.');
		
		// Truncate the string at the last dot, if there was one.
		if (PCdotExtension)
			*PCdotExtension = '\0';
			
		// Extract the output directory.
		SparamFolder = g_path_get_dirname(g_file_get_path(POfile));
		POparamFolder = g_file_new_for_path(SparamFolder);
		
		// Write the parameters we just computed.
		g_object_set(POconfig,
		             IMAGE2GB_PARAM_ASSET_NAME, SparamAssetName,
		             IMAGE2GB_PARAM_FOLDER, POparamFolder,
		             NULL);
		             
		// Show the plugin config dialog. If the user cancels it, do nothing
		// (this is not an error).
		if (! show_dialog(POprocedure, POconfig))
			return gimp_procedure_new_return_values(POprocedure, GIMP_PDB_CANCEL, NULL);
	}
	
	// Check the validity of the parameters that were given.
	if (! load_parameters(POconfig, ErunMode))
	{
		GError* POerror = NULL;
		
		g_set_error(&POerror, GIMP_PLUG_IN_ERROR, 0,
		            "Incorrect parameters.");
		            
		return gimp_procedure_new_return_values(POprocedure,
		                                        GIMP_PDB_CALLING_ERROR,
		                                        POerror);
	}
	
	// Continue as usual and try to export the image.
	EreturnValue = image2gb_export_image(POimage, ErunMode);
	
	return gimp_procedure_new_return_values(POprocedure, EreturnValue, NULL);
}

static void
image2gb_dispose(GObject* Pobject)
{
	// Free the memory that was allocated for the procedure list.
	if (PLprocedures)
	{
		g_list_free_full(PLprocedures, g_free);
		PLprocedures = NULL;
	}
	
	// Free memory of global variables (plugin parameters).
	g_free(SparamAssetName);
	g_free(SparamFolder);
	
	// Continue the chain by invoking the parent cleanup function.
	if (POparentClass && POparentClass->dispose)
		POparentClass->dispose(Pobject);
}

void
report_message(GimpRunMode ErunMode, const char* Smessage)
{
	if (ErunMode == GIMP_RUN_NONINTERACTIVE)
		g_message("%s\n", Smessage);
	else
		gimp_message(Smessage);
}

static gboolean
check_image(GimpImage* POimage, GimpRunMode ErunMode)
{
	GimpPalette* POpalette; /**< Pointer to the image palette (NULL if not indexed). */
	
	char* SformattedMessage; /**< String used to print error messages. */
	
	// Check that size is between 8x8 (1 tile) and 256x256 (32x32 tiles).
	if ((gimp_image_get_width(POimage) < IMAGE2GB_IMAGE_SIZE_MIN)
	    || (gimp_image_get_width(POimage) > IMAGE2GB_IMAGE_SIZE_MAX)
	    || (gimp_image_get_height(POimage) < IMAGE2GB_IMAGE_SIZE_MIN)
	    || (gimp_image_get_height(POimage) > IMAGE2GB_IMAGE_SIZE_MAX))
	{
		SformattedMessage = g_strdup_printf("ERROR: image size must be between %dx%d and %dx%d pixels.",
		                                    IMAGE2GB_IMAGE_SIZE_MIN, IMAGE2GB_IMAGE_SIZE_MIN,
		                                    IMAGE2GB_IMAGE_SIZE_MAX, IMAGE2GB_IMAGE_SIZE_MAX);
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return FALSE;
	}
	
	// Also, size should be a multiple of 8 (whole tiles).
	if (((gimp_image_get_width(POimage) % IMAGE2GB_TILE_SIZE) != 0)
	    || ((gimp_image_get_height(POimage) % IMAGE2GB_TILE_SIZE) != 0))
	{
		SformattedMessage = g_strdup_printf("ERROR: width and height must be multiples of %d.",
		                                    IMAGE2GB_TILE_SIZE);
		report_message(ErunMode, SformattedMessage);
		g_free(SformattedMessage);
		
		return FALSE;
	}
	
	// Check if the image is indexed, and if it is 4-color.
	POpalette = gimp_image_get_palette(POimage);
	
	if (POpalette)
	{
		if (gimp_palette_get_color_count(POpalette) != IMAGE2GB_IMAGE_COLORS)
		{
			report_message(ErunMode,
			               "ERROR: the image should be 4-color only. To be sure, use the palette(s) provided with this plugin.");
			               
			return FALSE;
		}
	}
	else
	{
		report_message(ErunMode, "ERROR: the image must be of 'INDEXED' type. Use Image->Mode->Indexed... to set it.");
		
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
show_dialog(GimpProcedure* POprocedure, GimpProcedureConfig* POconfig)
{
	GtkWidget* POdialog; /**< Plugin GUI window (modeless). */
	
	gimp_ui_init(IMAGE2GB_BINARY_NAME);
	
	POdialog = gimp_procedure_dialog_new(POprocedure,
	                                     GIMP_PROCEDURE_CONFIG(POconfig),
	                                     IMAGE2GB_DESCRIPTION_SHORT);
	gimp_procedure_dialog_fill(GIMP_PROCEDURE_DIALOG(POdialog), NULL);
	
	return gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(POdialog));
}

static gboolean
load_parameters(GimpProcedureConfig* POconfig, GimpRunMode ErunMode)
{
	char StruncatedName[IMAGE2GB_ASSET_NAME_MAX + 1]; /**< String buffer for shortening the asset name if needed. */
	
	GFile* POparamFolder = NULL; /**< Virtual file object for retrieving the "output path" parameter. */
	
	// Retrieve the parameters.
	g_object_get(POconfig,
	             IMAGE2GB_PARAM_ASSET_NAME, &SparamAssetName,
	             IMAGE2GB_PARAM_FOLDER, &POparamFolder,
	             IMAGE2GB_PARAM_BANK, &UIparamBank,
	             NULL);
	             
	// Check that the asset name is not empty.
	if (SparamAssetName == NULL || strlen((char*) SparamAssetName) == 0)
	{
		report_message(ErunMode, "ERROR: the asset name cannot be empty.");
		
		return FALSE;
	}
	
	// Truncate the asset name if longer than 32 characters.
	if (strlen((char*) SparamAssetName) > IMAGE2GB_ASSET_NAME_MAX)
	{
		// Copy the first 32 characters into the buffer.
		strncpy((char*) StruncatedName, (char*) SparamAssetName, IMAGE2GB_ASSET_NAME_MAX);
		
		// Make the string null-terminated.
		StruncatedName[IMAGE2GB_ASSET_NAME_MAX] = '\0';
		
		// Replace the original asset name with the truncated one.
		g_free(SparamAssetName);
		SparamAssetName = strdup((char*) StruncatedName);
	}
	
	// File arguments are stored as GFile objects. We save the path to a string.
	SparamFolder = g_file_get_path(POparamFolder);
	g_object_unref(POparamFolder);
	
	// Check that the output folder is not empty.
	if (SparamFolder == NULL || strlen((char*) SparamFolder) == 0)
	{
		report_message(ErunMode, "ERROR: the output path cannot be empty.");
		
		return FALSE;
	}
	
	// Check that the folder exists (we do not check for write permissions).
	if (!g_file_test((char*) SparamFolder, G_FILE_TEST_IS_DIR))
	{
		report_message(ErunMode, "ERROR: the output path must be an existing directory.");
		
		return FALSE;
	}
	
	return TRUE;
}
