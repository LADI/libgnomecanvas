/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 * Copyright 2001 Ximian, Inc.
 *
 * glade-canvas.c: support for canvas widgets in libglade.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Authors:
 *    Jacob Berkman <jacob@ximian.com>
 *    James Henstridge <james@daa.com.au>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>
#include <libgnomecanvas/gnome-canvas.h>

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

static GtkWidget *
canvas_new (GladeXML *xml, GType widget_type,
	    GladeWidgetInfo *info)
{
    const char *name, *value;
    gboolean aa = FALSE;
    gdouble sx1 = 0, sy1 = 0, sx2 = 100, sy2 = 100;
    gdouble pixels_per_unit = 1;
    int i;
    GtkWidget *wid;

    for (i = 0; i < info->n_properties; i++) {
	name = info->properties[i].name;
	value = info->properties[i].value;

	if (!strcmp(name, "anti_aliased"))
	    aa = (tolower (value[0] == 't') ||
		  tolower (value[0] == 'y') ||
		  strtol (value, NULL, 10));
	
	else if (!strcmp(name, "scroll_x1"))
	    sx1 = g_strtod(value, NULL);
	
	else if (!strcmp(name, "scroll_y1"))
	    sy1 = g_strtod(value, NULL);
	
	else if (!strcmp(name, "scroll_x2"))
	    sx2 = g_strtod(value, NULL);
	
	else if (!strcmp(name, "scroll_y2"))
	    sy2 = g_strtod(value, NULL);
	
	else if (!strcmp(name, "pixels_per_unit"))
	    pixels_per_unit = g_strtod(value, NULL);
    }

    g_message ("ignore warnings about the following: anti_aliased, scroll_x1, scroll_y1, scroll_x2, scroll_y2, pixels_per_unit");

    wid = glade_standard_build_widget (xml, widget_type, info);

    GNOME_CANVAS (wid)->aa = aa;
    gnome_canvas_set_scroll_region(GNOME_CANVAS(wid), sx1, sy1, sx2, sy2);
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(wid), pixels_per_unit);

    return wid;
}

void
glade_module_register_widgets (void)
{
    glade_register_widget (GNOME_TYPE_CANVAS, canvas_new, NULL, NULL);
    glade_provide ("canvas");
}
