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

    GList *deferred_props = NULL, *tmp;
    static GArray *props_array = NULL;
    GObjectClass *oclass;

    if (!props_array)
	props_array = g_array_new(FALSE, FALSE, sizeof(GParameter));

    /* we ref the class here as a slight optimisation */
    oclass = g_type_class_ref(widget_type);

    for (i = 0; i < info->n_properties; i++) {
	GParameter param = { NULL };
	GParamSpec *pspec;

	name = info->properties[i].name;
	value = info->properties[i].value;

	pspec = g_object_class_find_property(oclass, name);
	
	if (!pspec) {
	    if (!strcmp(name, "anti_aliased"))
		aa = (tolower (value[0] == 't') ||
		      tolower (value[0] == 'y') ||
		      atoi (value));
	    
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
	    else
		g_warning("unknown property `%s' for class `%s'",
			  name, g_type_name(widget_type));
	    continue;
	}

	/* this should catch all properties wanting a GtkWidget
         * subclass.  We also look for types that could hold a
         * GtkWidget in order to catch things like the
         * GtkAccelLabel::accel_object property.  Since we don't do
         * any handling of GObject or GtkObject directly in
         * glade_xml_set_value_from_string, this shouldn't be a
         * problem. */
	if (g_type_is_a(GTK_TYPE_WIDGET, G_PARAM_SPEC_VALUE_TYPE(pspec)) ||
	    g_type_is_a(G_PARAM_SPEC_VALUE_TYPE(pspec), GTK_TYPE_WIDGET)) {
	    deferred_props = g_list_prepend(deferred_props,
					    &info->properties[i]);
	    continue;
	}

	if (glade_xml_set_value_from_string(xml, pspec,
					    info->properties[i].value,
					    &param.value)) {
	    param.name = info->properties[i].name;
	    g_array_append_val(props_array, param);
	}
    }

    wid = g_object_newv(widget_type, props_array->len,
			(GParameter *)props_array->data);

    GNOME_CANVAS (wid)->aa = aa;
    gnome_canvas_set_scroll_region(GNOME_CANVAS(wid), sx1, sy1, sx2, sy2);
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(wid), pixels_per_unit);

    /* handle deferred properties */
    for (tmp = deferred_props; tmp; tmp = tmp->next) {
	GladeProperty *prop = tmp->data;

	glade_xml_handle_widget_prop(xml, wid, prop->name, prop->value);
    }
    g_list_free(deferred_props);

    g_array_set_size(props_array, 0);
    g_type_class_unref(oclass);


    return wid;
}

void
glade_module_register_widgets (void)
{
    glade_register_widget (GNOME_TYPE_CANVAS, canvas_new, NULL, NULL);
    glade_provide ("canvas");
}
