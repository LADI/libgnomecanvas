/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>
#include <gobject/genums.h>
#include <gobject/gboxed.h>

#include <libgnomecanvas/libgnomecanvas.h>

#include "libgnomecanvastypebuiltins.h"
#include "libgnomecanvastypebuiltins_vars.c"
#include "libgnomecanvastypebuiltins_evals.c"

void
libgnomecanvas_init (void)
{
	static gboolean initialized = FALSE;
	int             i;
	static struct {
		gchar           *type_name;
		GType           *type_id;
		GType            parent;
		gconstpointer    pointer1;
		gconstpointer    pointer2;
		gconstpointer    pointer3;
		gboolean         boolean1;
	} builtin_info [GNOME_TYPE_NUM_BUILTINS + 1] = {
#include "libgnomecanvastypebuiltins_ids.c"
		{ NULL }
	};

	if (initialized)
		return;
	else
		initialized = TRUE;

	g_type_init (G_TYPE_DEBUG_NONE);

	initialized = TRUE;

	for (i = 0; i < GNOME_TYPE_NUM_BUILTINS; i++) {
		GType type_id = G_TYPE_INVALID;

		g_assert (builtin_info[i].type_name != NULL);

		if (builtin_info[i].parent == G_TYPE_ENUM)
			type_id = g_enum_register_static (builtin_info[i].type_name, (GEnumValue *)builtin_info[i].pointer1);
		else if (builtin_info[i].parent == G_TYPE_FLAGS)
			type_id = g_flags_register_static (builtin_info[i].type_name, (GFlagsValue *)builtin_info[i].pointer1);
		else if (builtin_info[i].parent == G_TYPE_BOXED )
			type_id = g_boxed_type_register_static (builtin_info[i].type_name, (GBoxedInitFunc)builtin_info[1].pointer1, (GBoxedCopyFunc)builtin_info[i].pointer2, (GBoxedFreeFunc)builtin_info[i].pointer2, builtin_info[i].boolean1);
	    
		g_assert (type_id != G_TYPE_INVALID);
		(*builtin_info[i].type_id) = type_id;
	}
}
