/**
 * @file  image2gb.c
 * @brief GIMP plugin to export an image to Game Boy data (C code, for use with GBDK-2020) - implementation.
 */

#include "image2gb.h"

#include "image_export.h" // This one contains all export functionality.

// VARIABLES ///////////////////////////////////////////////////////////////////

/** Pointers to this plugin's functions. This variable will be read by GIMP.
 */
GimpPlugInInfo PLUG_IN_INFO = {NULL,
                               NULL,
                               image2gb_query,
                               image2gb_run
                              };

/** Stores the export parameters during execution.
 */
PluginExportOptions StructExportOptions = {0};

/** GTK text entry for choosing the asset name. It is global so we can read the
 *  value anywhere.
 */
GtkWidget* WtextName;

/** GTK button for choosing the destination folder. Contains the file chooser
 *  widget, it is global so we can read the value anywhere.
 */
GtkWidget* WbuttonFolder;

/** GTK spin button for choosing the ROM bank number. It is global so we can
 *  read the value anywhere.
 */
GtkWidget* WspinBank;

// FUNCTIONS ///////////////////////////////////////////////////////////////////

MAIN() // GIMP macro that declares a proper main() and initializes everything.

static void
image2gb_query(void)
{
	// Set what input parameters this plugin will use. First 5 are the minimum
	// ones needed for a file export plugin, you could add more at the end.
	static GimpParamDef Gparams[] = {{GIMP_PDB_INT32, "run-mode", "The run mode"},
		{GIMP_PDB_IMAGE, "image", "Input image"},
		{GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
		{GIMP_PDB_STRING, "filename", "The name of the file to save the image in"},
		{GIMP_PDB_STRING, "raw-filename", "The name of the file to save the image in"},
		{GIMP_PDB_INT32, "bank", "The ROM bank number to store the asset in (optional, default 0)"}
	};

	// Install the procedures in the PDB (Procedure DB). The same procedure can
	// not be used for both the menu and save handler, so this is done twice.
	gimp_install_procedure(IMAGE2GB_PROCEDURE_MENU,
	                       IMAGE2GB_DESCRIPTION_SHORT,
	                       IMAGE2GB_DESCRIPTION_LONG,
	                       IMAGE2GB_AUTHOR,
	                       IMAGE2GB_COPYRIGHT,
	                       IMAGE2GB_DATE,
	                       IMAGE2GB_MENU_NAME,
	                       IMAGE2GB_IMAGE_TYPES,
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(Gparams), 0,
	                       Gparams, NULL);
	gimp_install_procedure(IMAGE2GB_PROCEDURE_SAVE,
	                       IMAGE2GB_DESCRIPTION_SHORT,
	                       IMAGE2GB_DESCRIPTION_LONG,
	                       IMAGE2GB_AUTHOR,
	                       IMAGE2GB_COPYRIGHT,
	                       IMAGE2GB_DATE,
	                       IMAGE2GB_MENU_NAME,
	                       IMAGE2GB_IMAGE_TYPES,
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(Gparams), 0,
	                       Gparams, NULL);

	// Register the plugin, first part: menu entry.
	gimp_plugin_menu_register(IMAGE2GB_PROCEDURE_MENU, IMAGE2GB_MENU_PATH);
	// Associate the "text/plain" MIME file type (probably unnecesary?).
	gimp_register_file_handler_mime(IMAGE2GB_PROCEDURE_SAVE, IMAGE2GB_ASSOCIATED_MIME_TYPE);
	// Register the plugin, second part: file save handler. NOTE: the plugin
	// does not really save a file with this extension, it actually saves 2
	// files, a .c source and a .h header. This association is a just a shortcut
	// to make it easier for the user (and GIMP already has handlers for
	// exporting images to .c and .h extensions).
	gimp_register_save_handler(IMAGE2GB_PROCEDURE_SAVE, IMAGE2GB_ASSOCIATED_EXTENSION, "");
}

static void
image2gb_run(const gchar* Sname, gint InumParams, const GimpParam* Gparams, gint* InumReturnVals, GimpParam** GreturnVals)
{
	GimpRunMode GrunMode; /**< The run mode (GIMP_RUN_INTERACTIVE, GIMP_RUN_NONINTERACTIVE, GIMP_RUN_WITH_LAST_VALS). */
	gint32 IimageID; /**< ID of the image to export. */
	static GimpParam GreturnValues[1]; /**< Array of return values. */
	GimpPDBStatusType GreturnStatus = GIMP_PDB_SUCCESS; /**< Status return value of the plugin, usually success. */

	// Zero the parameter struct, just in case.
	memset(& StructExportOptions, 0, sizeof(StructExportOptions));

	// Get and store input parameters.
	GrunMode = Gparams[0].data.d_int32;
	IimageID = Gparams[1].data.d_int32;

	// Prepare the mandatory output values.
	* InumReturnVals = 1;
	* GreturnVals = GreturnValues;
	GreturnValues[0].type = GIMP_PDB_STATUS;

	// Before doing anything, check the validity of the image.
	if (! image2gb_check_image(IimageID))
		GreturnStatus = GIMP_PDB_CALLING_ERROR;

	// Try to get the last used export options.
	if (GreturnStatus == GIMP_PDB_SUCCESS)
	{
		if (! image2gb_load_parameters(IimageID))
			GrunMode = GIMP_RUN_INTERACTIVE; // Could not read? Force dialog.
	}

	// If invoked through "Export As" or a script, store the current choice of
	// destination (a full file name with path).
	if ((GreturnStatus == GIMP_PDB_SUCCESS) && (strcmp(Sname, IMAGE2GB_PROCEDURE_SAVE) == 0))
	{
		if ((Gparams[3].data.d_string != NULL) && (strlen(Gparams[3].data.d_string) != 0))
		{
			strcpy(StructExportOptions.name, g_path_get_basename(Gparams[3].data.d_string));
			strcpy(StructExportOptions.folder, g_path_get_dirname(Gparams[3].data.d_string));

			// We have to remove the extension from the file name.
			gint dotchar = strlen(StructExportOptions.name) - strlen(IMAGE2GB_ASSOCIATED_EXTENSION) - 1;
			StructExportOptions.name[dotchar] = '\0';
			// And make the first letter uppercase.
			StructExportOptions.name[0] = toupper(StructExportOptions.name[0]);
		}

		// If called by a script, get the ROM bank number parameter, if any.
		if ((GrunMode == GIMP_RUN_NONINTERACTIVE) && (InumParams == 6))
			StructExportOptions.bank = Gparams[5].data.d_int32;
	}

	// First time export, or invoked through menu entry? Show a dialog window to
	// let the user choose the parameters (it will be populated with the last
	// used values, if there were any).
	if ((GreturnStatus == GIMP_PDB_SUCCESS) && (GrunMode == GIMP_RUN_INTERACTIVE))
		GreturnStatus = image2gb_show_dialog();

	// Check that the asset name is not empty.
	if ((GreturnStatus == GIMP_PDB_SUCCESS) && (StructExportOptions.name[0] == '\0'))
	{
		g_message("The asset name can not be empty!\n");

		GreturnStatus = GIMP_PDB_CALLING_ERROR;
	}

	// Try to export the image.
	if (GreturnStatus == GIMP_PDB_SUCCESS)
		GreturnStatus = image2gb_export_image(IimageID, Gparams[2].data.d_drawable,
		                                      & StructExportOptions);

	// Save the parameters for the next invocation, using a parasite.
	if (GreturnStatus == GIMP_PDB_SUCCESS)
		image2gb_save_parameters(IimageID);

	GreturnValues[0].data.d_status = GreturnStatus;
}

static gboolean
image2gb_check_image(gint32 IimageID)
{
	// Check that size is between 8x8 (1 tile) and 256x256 (32x32 tiles).
	if (((gimp_image_width(IimageID) < IMAGE2GB_IMAGE_SIZE_MIN) || (gimp_image_width(IimageID) > IMAGE2GB_IMAGE_SIZE_MAX))
	    || ((gimp_image_height(IimageID) < IMAGE2GB_IMAGE_SIZE_MIN) || (gimp_image_height(IimageID) > IMAGE2GB_IMAGE_SIZE_MAX)))
	{
		g_message("Image size should be between %dx%d and %dx%d pixels.\n",
		          IMAGE2GB_IMAGE_SIZE_MIN, IMAGE2GB_IMAGE_SIZE_MIN,
		          IMAGE2GB_IMAGE_SIZE_MAX, IMAGE2GB_IMAGE_SIZE_MAX);

		return FALSE;
	}

	// Also, size should be a multiple of 8 (whole tiles).
	if (((gimp_image_width(IimageID) % IMAGE2GB_TILE_SIZE) != 0)
	    || ((gimp_image_height(IimageID) % IMAGE2GB_TILE_SIZE) != 0))
	{
		g_message("Both width and height should be multiples of %d.\n", IMAGE2GB_TILE_SIZE);

		return FALSE;
	}

	// We know the image is indexed, but we need to check it is 4-color.
	gint ncolors;
	gimp_image_get_colormap(IimageID, & ncolors);

	if (ncolors != 4)
	{
		g_message("The image should be 4-color only. To be sure, use the Game Boy palette provided with this plugin.\n");

		return FALSE;
	}

	return TRUE;
}

static GimpPDBStatusType
image2gb_show_dialog(void)
{
	GimpPDBStatusType GreturnStatus = GIMP_PDB_SUCCESS; /**< Return value. */

	// GTK widgets for composing the user interface, they should all be self
	// explanatory.
	GtkWidget* WdialogWindow;
	GtkWidget* WvBoxWindow;
	GtkWidget* WhBoxName;
	GtkWidget* WlabelName;
	GtkWidget* WhBoxFolder;
	GtkWidget* WlabelFolder;
	GtkWidget* WhBoxBank;
	GtkWidget* WlabelBank;

	// Initialize GTK, plugin would crash otherwise.
	gimp_ui_init(IMAGE2GB_BINARY_NAME, FALSE);

	// This is how image export dialogs should be created and registered.
	WdialogWindow = gimp_export_dialog_new(IMAGE2GB_MENU_NAME, IMAGE2GB_BINARY_NAME, IMAGE2GB_PROCEDURE_SAVE);
	g_signal_connect(WdialogWindow, "response", G_CALLBACK(image2gb_dialog_response), & GreturnStatus);
	g_signal_connect(WdialogWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_window_set_resizable(GTK_WINDOW(WdialogWindow), FALSE);

	// Get the GTK vertical box of the dialog, where all controls should go.
	WvBoxWindow = gimp_export_dialog_get_content_area(WdialogWindow);

	// Widget controls group: asset name.
	WhBoxName = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	WlabelName = gtk_label_new("Asset name (C naming rules):");
	gtk_box_pack_start(GTK_BOX(WhBoxName), WlabelName, FALSE, FALSE, 5);
	gtk_widget_show(WlabelName);

	WtextName = gtk_entry_new();
	gtk_widget_set_tooltip_text(WtextName, "Keep it short and a valid C identifier, it will be the base name for the variables.");
	gtk_entry_set_max_length(GTK_ENTRY(WtextName), (IMAGE2GB_ASSET_NAME_MAX_LENGTH - 4)); // 32 - "Bkg" or "Map" prefix and '\0'.
	gtk_widget_set_size_request(WtextName, 250, -1);
	gtk_entry_set_text(GTK_ENTRY(WtextName), StructExportOptions.name);
	gtk_box_pack_start(GTK_BOX(WhBoxName), WtextName, TRUE, TRUE, 5);
	gtk_widget_show(WtextName);

	// Widget controls group: destination folder.
	WhBoxFolder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	WlabelFolder = gtk_label_new("Destination folder:");
	gtk_box_pack_start(GTK_BOX(WhBoxFolder), WlabelFolder, FALSE, FALSE, 5);
	gtk_widget_show(WlabelFolder);

	WbuttonFolder = gtk_file_chooser_button_new("Select a folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_set_tooltip_text(WbuttonFolder, "The .c and .h files will be created in this folder.");
	gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(WbuttonFolder), StructExportOptions.folder);

	gtk_box_pack_start(GTK_BOX(WhBoxFolder), WbuttonFolder, TRUE, TRUE, 5);
	gtk_widget_show(WbuttonFolder);

	// Widget controls group: ROM bank number.
	WhBoxBank = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	WlabelBank = gtk_label_new("ROM bank number (optional):");
	gtk_box_pack_start(GTK_BOX(WhBoxBank), WlabelBank, FALSE, FALSE, 5);
	gtk_widget_show(WlabelBank);

	WspinBank = gtk_spin_button_new_with_range(0, 127, 1);
	gtk_widget_set_tooltip_text(WspinBank, "Set it to 0 for using the default bank.");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(WspinBank), StructExportOptions.bank);
	gtk_box_pack_start(GTK_BOX(WhBoxBank), WspinBank, FALSE, FALSE, 5);
	gtk_widget_show(WspinBank);

	// Put all boxes inside the dialog box.
	gtk_box_pack_start(GTK_BOX(WvBoxWindow), WhBoxName, TRUE, TRUE, 5);
	gtk_widget_show(WhBoxName);
	gtk_box_pack_start(GTK_BOX(WvBoxWindow), WhBoxFolder, TRUE, TRUE, 5);
	gtk_widget_show(WhBoxFolder);
	gtk_box_pack_start(GTK_BOX(WvBoxWindow), WhBoxBank, TRUE, TRUE, 5);
	gtk_widget_show(WhBoxBank);

	gtk_widget_show(WdialogWindow);

	// GTK loop, now just wait for user action (this is a blocking call).
	gtk_main();

	return GreturnStatus;
}

static void
image2gb_dialog_response(GtkWidget* Wwidget, gint IresponseID, gpointer Pdata)
{
	GimpPDBStatusType* GreturnStatus = Pdata; /**< Pointer to return status variable. */

	// GTK_RESPONSE_OK means "Export" was clicked. GIMP_PDB_CANCEL... guess.
	if (IresponseID == GTK_RESPONSE_OK)
	{
		(* GreturnStatus) = GIMP_PDB_SUCCESS;

		// Save the values of the current export options.
		strcpy(StructExportOptions.name, gtk_entry_get_text(GTK_ENTRY(WtextName)));

		if (gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(WbuttonFolder)) != NULL)
			strcpy(StructExportOptions.folder, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(WbuttonFolder)));

		StructExportOptions.bank = gtk_spin_button_get_value(GTK_SPIN_BUTTON(WspinBank));
	}
	else (* GreturnStatus) = GIMP_PDB_CANCEL;

	// This will automatically free memory of all widgets.
	gtk_widget_destroy(Wwidget);
}

static gboolean
image2gb_load_parameters(gint32 IimageID)
{
	GimpParasite* Gparasite; /**< Persistent parameters (stored values of previous export). */

	Gparasite = gimp_image_get_parasite(IimageID, IMAGE2GB_PARASITE);

	if (Gparasite)
	{
		const PluginExportOptions* StructSavedOptions = gimp_parasite_data(Gparasite);

		// Recover the values.
		strcpy(StructExportOptions.name, StructSavedOptions->name);
		strcpy(StructExportOptions.folder, StructSavedOptions->folder);
		StructExportOptions.bank = StructSavedOptions->bank;

		gimp_parasite_free(Gparasite);

		return TRUE;
	}

	return FALSE;
}

static void
image2gb_save_parameters(gint32 IimageID)
{
	GimpParasite* Gparasite; /**< Persistent parameters (stored values for subsequent exports). */

	// Remove the current one.
	gimp_image_detach_parasite(IimageID, IMAGE2GB_PARASITE);

	Gparasite = gimp_parasite_new(IMAGE2GB_PARASITE, 0, sizeof(StructExportOptions), & StructExportOptions);
	gimp_image_attach_parasite(IimageID, Gparasite);
	gimp_parasite_free(Gparasite);
}
