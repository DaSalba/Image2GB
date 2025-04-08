/**
 * @file  image2gb.h
 * @brief GIMP plugin to export an image to Game Boy data (C code, for use with GBDK-2020) - header.
 */

#pragma once

#include <libgimp/gimp.h>

// DEFINITIONS /////////////////////////////////////////////////////////////////

/** Unused, but needed to adhere to GIMP 3's GObject structure.
 */
struct _Image2GB
{
	GimpPlugIn parent_instance;
};

// VARIABLES ///////////////////////////////////////////////////////////////////

extern char* SparamAssetName; /**< Current value of the "asset name" parameter. */
extern char* SparamFolder;    /**< Current value of the "output path" parameter. */

extern unsigned int UIparamBank; /**< Current value of the "bank number" parameter. */

// FUNCTIONS ///////////////////////////////////////////////////////////////////

// GObject convenience macro for declaring stuff.
G_DECLARE_FINAL_TYPE(Image2GB, image2gb, IMAGE2GB,, GimpPlugIn)

/** Generic initialization for this plugin class, called before instancing it.
 *
 * \param[in] POclass Pointer to the plugin class.
 */
static void
image2gb_class_init(Image2GBClass* POclass);

/** Initializes a particular instance of this plugin.
 *
 * \param[in] POinstance Pointer to the plugin instance to initialize.
 */
static void
image2gb_init(Image2GB* POinstance);

/** Returns information about the procedures provided by this plugin.
 *
 * \param[in] POplugin Pointer to this GIMP plugin.
 *
 * \return Pointer to the list of procedures.
 */
static GList*
image2gb_query_procedures(GimpPlugIn* POplugin);

/** Creates and returns an instance of the requested procedure.
 *
 * \param[in] POplugin Pointer to this GIMP plugin.
 * \param[in] Sname    Name of the procedure to create.
 *
 * \return Pointer to the requested procedure.
 */
static GimpProcedure*
image2gb_create_procedure(GimpPlugIn* POplugin,
                          const gchar* Sname);

/** Runs the "menu" (GUI export window in Tools/) procedure of this plugin.
 *
 * \param[in] POprocedure Pointer to the procedure.
 * \param[in] ErunMode    The run mode (interactive or not).
 * \param[in] POimage     Pointer to the current image.
 * \param[in] Adrawables  Array of selected drawables.
 * \param[in] POconfig    Pointer to the procedure config.
 * \param[in] PrunData    Pointer to the run data.
 *
 * \return Pointer to the list of return values.
 */
extern GimpValueArray*
image2gb_run_menu(GimpProcedure*       POprocedure,
                  GimpRunMode          ErunMode,
                  GimpImage*           POimage,
                  GimpDrawable**       Adrawables,
                  GimpProcedureConfig* POconfig,
                  gpointer             PrunData);

/** Runs the "save" (Export... option in File/) procedure of this plugin.
 *
 * \param[in] POprocedure Pointer to the procedure.
 * \param[in] ErunMode    The run mode (interactive or not).
 * \param[in] POimage     Pointer to the current image.
 * \param[in] POfile      Pointer to the chosen output file.
 * \param[in] POoptions   Pointer to the export options.
 * \param[in] POmetadata  Pointer to the metadata.
 * \param[in] POconfig    Pointer to the procedure config.
 * \param[in] PrunData    Pointer to the run data.
 *
 * \return Pointer to the list of return values.
 */
extern GimpValueArray*
image2gb_run_save(GimpProcedure*       POprocedure,
                  GimpRunMode          ErunMode,
                  GimpImage*           POimage,
                  GFile*               POfile,
                  GimpExportOptions*   POoptions,
                  GimpMetadata*        POmetadata,
                  GimpProcedureConfig* POconfig,
                  gpointer             PrunData);

/** Performs plugin cleanup.
 *
 * \param[in] POinstance Pointer to the plugin instance to clean.
 */
static void
image2gb_dispose(GObject* POinstance);

/** Shows a message to the user, either in console if running non-interactively
 *  or with a modal GUI window otherwise.
 *
 * \param[in] ErunMode The run mode (interactive or not).
 * \param[in] Smessage String containing the message to show.
 */
extern void
report_message(GimpRunMode ErunMode, const char* Smessage);
