/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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
/* Polygon item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *         Rusty Conover <rconover@bangtail.net>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_svp_vpath_stroke.h"
#include "libgnomecanvas.h"

#include "gnome-canvas-shape.h"


#define NUM_STATIC_POINTS 256	/* Number of static points to use to avoid allocating arrays */


enum {
	PROP_0,
	PROP_POINTS,
};


static void gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class);
static void gnome_canvas_polygon_init       (GnomeCanvasPolygon      *poly);
static void gnome_canvas_polygon_destroy    (GtkObject               *object);
static void gnome_canvas_polygon_set_property (GObject              *object,
					       guint                 param_id,
					       const GValue         *value,
					       GParamSpec           *pspec);
static void gnome_canvas_polygon_get_property (GObject              *object,
					       guint                 param_id,
					       GValue               *value,
					       GParamSpec           *pspec);

static void gnome_canvas_polygon_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

static GnomeCanvasItemClass *parent_class;

GtkType
gnome_canvas_polygon_get_type (void)
{
	static GtkType polygon_type = 0;

	if (!polygon_type) {
		GtkTypeInfo polygon_info = {
			"GnomeCanvasPolygon",
			sizeof (GnomeCanvasPolygon),
			sizeof (GnomeCanvasPolygonClass),
			(GtkClassInitFunc) gnome_canvas_polygon_class_init,
			(GtkObjectInitFunc) gnome_canvas_polygon_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		polygon_type = gtk_type_unique (gnome_canvas_shape_get_type (), &polygon_info);
	}

	return polygon_type;
}

static void
gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_shape_get_type ());

	gobject_class->set_property = gnome_canvas_polygon_set_property;
	gobject_class->get_property = gnome_canvas_polygon_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_POINTS,
                 g_param_spec_boxed ("points", NULL, NULL,
				     GNOME_TYPE_CANVAS_POINTS,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_polygon_destroy;

	item_class->update = gnome_canvas_polygon_update;
}

static void
gnome_canvas_polygon_init (GnomeCanvasPolygon *poly)
{
	poly->path_def = NULL;
}

static void
gnome_canvas_polygon_destroy (GtkObject *object)
{
	GnomeCanvasPolygon *poly;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	poly = GNOME_CANVAS_POLYGON (object);

	/* remember, destroy can be run multiple times! */

	if(poly->path_def)
		gnome_canvas_path_def_unref(poly->path_def);

	poly->path_def = NULL;


	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
set_points (GnomeCanvasPolygon *poly, GnomeCanvasPoints *points)
{
	int i, duplicate;


	if(poly->path_def)
		gnome_canvas_path_def_unref(poly->path_def);

	if(!points) {
		poly->path_def = gnome_canvas_path_def_new();
		gnome_canvas_shape_set_path_def (GNOME_CANVAS_SHAPE (poly), poly->path_def);
		return;
	}


	/* Optomize the path def to the number of points */
	poly->path_def = gnome_canvas_path_def_new_sized(points->num_points+1);

	/* See if we need to duplicate the first point */
	duplicate = ((points->coords[0] != points->coords[2 * points->num_points - 2])
		     || (points->coords[1] != points->coords[2 * points->num_points - 1]));

	
	gnome_canvas_path_def_moveto(poly->path_def, points->coords[0], points->coords[1]);
	
	for(i = 1; i < points->num_points; i++) {
		gnome_canvas_path_def_lineto(poly->path_def, 
					     points->coords[i*2], 
					     points->coords[(i*2)+1]);
	}

	if (duplicate) {
		gnome_canvas_path_def_closepath(poly->path_def);		
	}

	gnome_canvas_shape_set_path_def (GNOME_CANVAS_SHAPE (poly), poly->path_def);
}


static void
gnome_canvas_polygon_set_property (GObject              *object,
				   guint                 param_id,
				   const GValue         *value,
				   GParamSpec           *pspec)
{
	GnomeCanvasItem *item;
	GnomeCanvasPolygon *poly;
	GnomeCanvasPoints *points;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	item = GNOME_CANVAS_ITEM (object);
	poly = GNOME_CANVAS_POLYGON (object);

	switch (param_id) {
	case PROP_POINTS:
		points = g_value_get_boxed (value);

		set_points (poly, points);

		gnome_canvas_item_request_update (item);
		break;
 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
gnome_canvas_polygon_get_property (GObject              *object,
				   guint                 param_id,
				   GValue               *value,
				   GParamSpec           *pspec)
{
	GnomeCanvasPolygon *poly;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	poly = GNOME_CANVAS_POLYGON (object);

	switch (param_id) {
	case PROP_POINTS:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
gnome_canvas_polygon_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	/* Since the path has already been defined just pass the update up. */

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);
}
