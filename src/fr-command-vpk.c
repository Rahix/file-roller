/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-vpk.h"

#define ZIP_SPECIAL_CHARACTERS "[]*?!^-\\"

G_DEFINE_TYPE (FrCommandVpk, fr_command_vpk, FR_TYPE_COMMAND)


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData    *fdata;
	FrCommand   *comm = FR_COMMAND (data);
	char       **fields;
	char        *name;
	char        *size_string;
	const char  *name_field;
	gint         line_l;

	g_return_if_fail (line != NULL);

	/* check whether unzip gave the empty archive warning. */

	//if (FR_COMMAND_VPK (comm)->is_empty)
	//	return;

	line_l = strlen (line);

	if (line_l == 0)
		return;

	/* ignore lines that do not describe a file or a
	 * directory. */
	//if ((line[0] != '?') && (line[0] != 'd') && (line[0] != '-'))
	//	return;

	/**/

	fdata = file_data_new ();

	//fields = _g_str_split_line (line, 7);
	name = strtok(line, " ");
	// skip crc
	size_string = strtok(NULL, " ");

	fdata->size = g_ascii_strtoull (size_string + 5, NULL, 10);
	fdata->modified = NULL; //mktime_from_string (fields[6]);
	//fdata->encrypted = (*fields[4] == 'B') || (*fields[4] == 'T');
	//g_strfreev (fields);

	/* Full path */

	name_field = name; //fields[0]; //_g_str_get_last_field (line, 8);

	if (*name_field == '/') {
		fdata->full_path = g_strdup (name_field);
		fdata->original_path = fdata->full_path;
	} else {
		fdata->full_path = g_strconcat ("/", name_field, NULL);
		fdata->original_path = fdata->full_path + 1;
	}

	fdata->link = NULL;

	//fdata->dir = line[0] == 'd';
	if (fdata->dir)
		fdata->name = _g_path_get_dir_name (fdata->full_path);
	else
		fdata->name = g_strdup (_g_path_get_basename (fdata->full_path));
	fdata->path = _g_path_remove_level (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_archive_add_file (FR_ARCHIVE (comm), fdata);
}


static void
list__begin (gpointer data)
{
	FrCommandVpk *comm = data;
}


static gboolean
fr_command_vpk_list (FrCommand  *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "vpk");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "-la");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	return TRUE;
}


static void
process_line__common (char     *line,
		      gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);
	FrArchive *archive = FR_ARCHIVE (comm);

	if (line == NULL)
		return;

	if (fr_archive_progress_get_total_files (archive) > 1)
		fr_archive_progress (archive, fr_archive_progress_inc_completed_files (archive, 1));
	else
		fr_archive_message (archive, line);
}

static void
fr_command_vpk_extract (FrCommand  *comm,
			const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_line__common,
				      comm);


	//if (overwrite)
		//fr_process_add_arg (comm->process, "-o");
	//else
		//fr_process_add_arg (comm->process, "-n");
	//if (skip_older)
		//fr_process_add_arg (comm->process, "-u");
	//if (junk_paths)
		//fr_process_add_arg (comm->process, "-j");
	//add_password_arg (comm, FR_ARCHIVE (comm)->password);


	for (scan = file_list; scan; scan = scan->next) {
		char *escaped;

		fr_process_begin_command (comm->process, "vpk");
		fr_process_add_arg (comm->process, "-x");
		if (dest_dir != NULL) {
			fr_process_add_arg (comm->process, dest_dir);
		} else {
			fr_process_add_arg (comm->process, "/tmp");
		}

 		escaped = _g_str_escape (scan->data, ZIP_SPECIAL_CHARACTERS);
 		fr_process_add_arg (comm->process, "-f");
 		fr_process_add_arg (comm->process, escaped);
 		g_free (escaped);
		// Name of the archive
		fr_process_add_arg (comm->process, comm->filename);
		fr_process_end_command (comm->process);
	}
}


/*
static void
fr_command_zip_test (FrCommand   *comm)
{
	fr_process_begin_command (comm->process, "unzip");
	fr_process_add_arg (comm->process, "-t");
	add_password_arg (comm, FR_ARCHIVE (comm)->password);
	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}*/


static void
fr_command_zip_handle_error (FrCommand *comm,
			     FrError   *error)
{
	if (error->type == FR_ERROR_NONE)
		return;

	if ((error->status == 82) || (error->status == 5))
		fr_error_take_gerror (error, g_error_new_literal (FR_ERROR, FR_ERROR_ASK_PASSWORD, ""));
	else {
		int i;

		for (i = 1; i <= 2; i++) {
			GList *output;
			GList *scan;

			output = (i == 1) ? comm->process->err.raw : comm->process->out.raw;

			for (scan = g_list_last (output); scan; scan = scan->prev) {
				char *line = scan->data;

				if (strstr (line, "incorrect password") != NULL) {
					fr_error_take_gerror (error, g_error_new_literal (FR_ERROR, FR_ERROR_ASK_PASSWORD, ""));
					return;
				}
			}
		}

		/* ignore warnings */

		if (error->status <= 1)
			fr_error_clear_gerror (error);
	}
}


const char *vpk_mime_type[] = { "application/x-vpk",
				NULL };


static const char **
fr_command_vpk_get_mime_types (FrArchive *archive)
{
	return vpk_mime_type;
}


static FrArchiveCap
fr_command_vpk_get_capabilities (FrArchive  *archive,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrArchiveCap capabilities;

	capabilities = FR_ARCHIVE_CAN_STORE_MANY_FILES;
	if (_g_program_is_available ("vpk", check_command))
		capabilities |= FR_ARCHIVE_CAN_READ;

	return capabilities;
}


static const char *
fr_command_vpk_get_packages (FrArchive  *archive,
			     const char *mime_type)
{
	return PACKAGES ("vpk");
}


static void
fr_command_vpk_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_VPK (object));

	if (G_OBJECT_CLASS (fr_command_vpk_parent_class)->finalize)
		G_OBJECT_CLASS (fr_command_vpk_parent_class)->finalize (object);
}


static void
fr_command_vpk_class_init (FrCommandVpkClass *klass)
{
	GObjectClass   *gobject_class;
	FrArchiveClass *archive_class;
	FrCommandClass *command_class;

	fr_command_vpk_parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = fr_command_vpk_finalize;

	archive_class = FR_ARCHIVE_CLASS (klass);
	archive_class->get_mime_types   = fr_command_vpk_get_mime_types;
	archive_class->get_capabilities = fr_command_vpk_get_capabilities;
	archive_class->get_packages     = fr_command_vpk_get_packages;

	command_class = FR_COMMAND_CLASS (klass);
	command_class->list             = fr_command_vpk_list;
	//command_class->add              = fr_command_vpk_add;
	//command_class->delete           = fr_command_vpk_delete;
	command_class->extract          = fr_command_vpk_extract;
	//command_class->test             = fr_command_vpk_test;
	//command_class->handle_error     = fr_command_vpk_handle_error;
}


static void
fr_command_vpk_init (FrCommandVpk *self)
{
	FrArchive *base = FR_ARCHIVE (self);

	base->propAddCanUpdate             = TRUE;
	base->propAddCanReplace            = TRUE;
	base->propAddCanStoreFolders       = TRUE;
	base->propAddCanStoreLinks         = FALSE;
	base->propExtractCanAvoidOverwrite = TRUE;
	base->propExtractCanSkipOlder      = FALSE;
	base->propExtractCanJunkPaths      = FALSE;
	base->propPassword                 = FALSE;
	base->propTest                     = FALSE;
}
