/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
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
 */

/* this file is only built if GNOME Canvas support is enabled */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>
#include <libgnomecanvas/gnome-canvas.h>

static GladeWidgetBuildData widget_data[] = {
    { "GnomeCanvas", glade_standard_build_widget, NULL, gnome_canvas_get_type },
    { NULL, NULL, NULL, 0, 0 }
};

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
    glade_provide ("canvas");
    glade_register_widgets (widget_data);
}
