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
/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Rusty Conover <rconover@bangtail.net>
 */

#include <config.h>
#include <math.h>
#include "gnome-canvas-rect-ellipse.h"
#include "gnome-canvas-util.h"
#include "gnome-canvas-shape.h"


#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_rgb_svp.h"

/* Base class for rectangle and ellipse item types */

#define noVERBOSE

enum {
	PROP_0,
	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2,
};


static void gnome_canvas_re_class_init (GnomeCanvasREClass *class);
static void gnome_canvas_re_init       (GnomeCanvasRE      *re);
static void gnome_canvas_re_destroy    (GtkObject          *object);
static void gnome_canvas_re_set_property (GObject              *object,
					  guint                 param_id,
					  const GValue         *value,
					  GParamSpec           *pspec);
static void gnome_canvas_re_get_property (GObject              *object,
					  guint                 param_id,
					  GValue               *value,
					  GParamSpec           *pspec);

static void gnome_canvas_re_update_shared      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

static void gnome_canvas_rect_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void gnome_canvas_ellipse_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

static GnomeCanvasItemClass *re_parent_class;


GtkType
gnome_canvas_re_get_type (void)
{
	static GtkType re_type = 0;

	if (!re_type) {
		GtkTypeInfo re_info = {
			"GnomeCanvasRE",
			sizeof (GnomeCanvasRE),
			sizeof (GnomeCanvasREClass),
			(GtkClassInitFunc) gnome_canvas_re_class_init,
			(GtkObjectInitFunc) gnome_canvas_re_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		re_type = gtk_type_unique (gnome_canvas_shape_get_type (), &re_info);
	}

	return re_type;
}

static void
gnome_canvas_re_class_init (GnomeCanvasREClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	re_parent_class = gtk_type_class (gnome_canvas_shape_get_type ());

	gobject_class->set_property = gnome_canvas_re_set_property;
	gobject_class->get_property = gnome_canvas_re_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_X1,
                 g_param_spec_double ("x1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y1,
                 g_param_spec_double ("y1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X2,
                 g_param_spec_double ("x2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y2,
                 g_param_spec_double ("y2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_re_destroy;
}

static void
gnome_canvas_re_init (GnomeCanvasRE *re)
{
	re->x1 = 0.0;
	re->y1 = 0.0;
	re->x2 = 0.0;
	re->y2 = 0.0;
	re->path_dirty = 0;
}

static void
gnome_canvas_re_destroy (GtkObject *object)
{
	GnomeCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	re = GNOME_CANVAS_RE (object);

	if (GTK_OBJECT_CLASS (re_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (re_parent_class)->destroy) (object);
}

static void
gnome_canvas_re_set_property (GObject              *object,
			      guint                 param_id,
			      const GValue         *value,
			      GParamSpec           *pspec)
{
	GnomeCanvasItem *item;
	GnomeCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	item = GNOME_CANVAS_ITEM (object);
	re = GNOME_CANVAS_RE (object);

	switch (param_id) {
	case PROP_X1:
		re->x1 = g_value_get_double (value);
		re->path_dirty = 1;
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y1:
		re->y1 = g_value_get_double (value);
		re->path_dirty = 1;
		gnome_canvas_item_request_update (item);
		break;

	case PROP_X2:
		re->x2 = g_value_get_double (value);
		re->path_dirty = 1;
		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y2:
		re->y2 = g_value_get_double (value);
		re->path_dirty = 1;
		gnome_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_re_get_property (GObject              *object,
			      guint                 param_id,
			      GValue               *value,
			      GParamSpec           *pspec)
{
	GnomeCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	re = GNOME_CANVAS_RE (object);

	switch (param_id) {
	case PROP_X1:
		g_value_set_double (value,  re->x1);
		break;

	case PROP_Y1:
		g_value_set_double (value,  re->y1);
		break;

	case PROP_X2:
		g_value_set_double (value,  re->x2);
		break;

	case PROP_Y2:
		g_value_set_double (value,  re->y2);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_re_update_shared (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasRE *re;

	re = GNOME_CANVAS_RE (item);

	if (re_parent_class->update)
		(* re_parent_class->update) (item, affine, clip_path, flags);
}


/* Rectangle item */
static void gnome_canvas_rect_class_init (GnomeCanvasRectClass *class);



GtkType
gnome_canvas_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		GtkTypeInfo rect_info = {
			"GnomeCanvasRect",
			sizeof (GnomeCanvasRect),
			sizeof (GnomeCanvasRectClass),
			(GtkClassInitFunc) gnome_canvas_rect_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		rect_type = gtk_type_unique (gnome_canvas_re_get_type (), &rect_info);
	}

	return rect_type;
}

static void
gnome_canvas_rect_class_init (GnomeCanvasRectClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->update = gnome_canvas_rect_update;
}

static void
gnome_canvas_rect_update (GnomeCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags)
{	GnomeCanvasRE *re;	

	GnomeCanvasPathDef *path_def;

	re = GNOME_CANVAS_RE(item);

	if(re->path_dirty) {		
		path_def = gnome_canvas_path_def_new();
		
		gnome_canvas_path_def_moveto(path_def, re->x1, re->y1);
		gnome_canvas_path_def_lineto(path_def, re->x2, re->y1);
		gnome_canvas_path_def_lineto(path_def, re->x2, re->y2);
		gnome_canvas_path_def_lineto(path_def, re->x1, re->y2);
		gnome_canvas_path_def_lineto(path_def, re->x1, re->y1);		
		gnome_canvas_path_def_closepath_current(path_def);		
		gnome_canvas_shape_set_path_def(G_OBJECT(item), path_def);
		gnome_canvas_path_def_unref(path_def);
		re->path_dirty = 0;
	}

	gnome_canvas_re_update_shared (item, affine, clip_path, flags);

}

/* Ellipse item */


static void gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class);


GtkType
gnome_canvas_ellipse_get_type (void)
{
	static GtkType ellipse_type = 0;

	if (!ellipse_type) {
		GtkTypeInfo ellipse_info = {
			"GnomeCanvasEllipse",
			sizeof (GnomeCanvasEllipse),
			sizeof (GnomeCanvasEllipseClass),
			(GtkClassInitFunc) gnome_canvas_ellipse_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		ellipse_type = gtk_type_unique (gnome_canvas_re_get_type (), &ellipse_info);
	}

	return ellipse_type;
}

static void
gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->update = gnome_canvas_ellipse_update;
}

#define N_PTS 90

static void
gnome_canvas_ellipse_update (GnomeCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags) {
	GnomeCanvasPathDef *path_def;
	GnomeCanvasRE *re;
	int i = 0;
	double th;

	re = GNOME_CANVAS_RE(item);

	if(re->path_dirty) {
		path_def = gnome_canvas_path_def_new();

		th = (2 * M_PI * i) / N_PTS;
		
		gnome_canvas_path_def_moveto(path_def, (re->x1 + re->x2) * 0.5 + (re->x2 - re->x1) * 0.5 * cos (th), (re->y1 + re->y2) * 0.5 - (re->y2 - re->y1) * 0.5 * sin (th));       
		
		for (i = 1; i < N_PTS + 1; i++) {
			th = (2 * M_PI * i) / N_PTS;		
			gnome_canvas_path_def_lineto(path_def,
						     (re->x1+ re->x2) * 0.5 + (re->x2 - re->x1) * 0.5 * cos (th), 
						     (re->y1 + re->y2) * 0.5 - (re->y2 - re->y1) * 0.5 * sin (th));	
		}
		
		gnome_canvas_path_def_closepath_current(path_def);
		
		gnome_canvas_shape_set_path_def(G_OBJECT(item), path_def);
		gnome_canvas_path_def_unref(path_def);
		re->path_dirty = 0;
	}

		
	gnome_canvas_re_update_shared(item, affine, clip_path, flags);
}
