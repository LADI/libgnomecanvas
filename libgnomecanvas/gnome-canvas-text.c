/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * $Id$
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
/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 * Port to Pango co-done by Gergõ Érdi <cactus@cactus.rulez.org>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "gnome-canvas-text.h"
#include <pango/pangoft2.h>

#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb_a_affine.h"
#include "libart_lgpl/art_rgb.h"
#include "libart_lgpl/art_rgb_bitmap_affine.h"
#include "gnome-canvas-util.h"
#include "gnome-canvas-i18n.h"



/* Object argument IDs */
enum {
	PROP_0,

	/* Text contents */
	PROP_TEXT,
	PROP_MARKUP,

	/* Position */
	PROP_X,
	PROP_Y,

	/* Font */
	PROP_FONT,
	PROP_FONT_DESC,
	PROP_FAMILY, PROP_FAMILY_SET,
	
	/* Style */
	PROP_ATTRIBUTES,
	PROP_STYLE,         PROP_STYLE_SET,
	PROP_VARIANT,       PROP_VARIANT_SET,
	PROP_WEIGHT,        PROP_WEIGHT_SET,
	PROP_STRETCH,	    PROP_STRETCH_SET,
	PROP_SIZE,          PROP_SIZE_SET,
	PROP_SIZE_POINTS,
	PROP_STRIKETHROUGH, PROP_STRIKETHROUGH_SET,
	PROP_UNDERLINE,     PROP_UNDERLINE_SET,
	PROP_RISE,          PROP_RISE_SET,
	PROP_SCALE,         PROP_SCALE_SET,

	/* Clipping */
	PROP_ANCHOR,
	PROP_JUSTIFICATION,
	PROP_CLIP_WIDTH,
	PROP_CLIP_HEIGHT,
	PROP_CLIP,
	PROP_X_OFFSET,
	PROP_Y_OFFSET,

	/* Coloring */
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_FILL_STIPPLE,

	/* Rendered size accessors */
	PROP_TEXT_WIDTH,
	PROP_TEXT_HEIGHT
};

struct _GnomeCanvasTextPrivate {
	guint render_dirty : 1;
	FT_Bitmap bitmap;
};


static void gnome_canvas_text_class_init (GnomeCanvasTextClass *class);
static void gnome_canvas_text_init (GnomeCanvasText *text);
static void gnome_canvas_text_destroy (GtkObject *object);
static void gnome_canvas_text_set_property (GObject            *object,
					    guint               param_id,
					    const GValue       *value,
					    GParamSpec         *pspec);
static void gnome_canvas_text_get_property (GObject            *object,
					    guint               param_id,
					    GValue             *value,
					    GParamSpec         *pspec);

static void gnome_canvas_text_update (GnomeCanvasItem *item, double *affine,
				      ArtSVP *clip_path, int flags);
static void gnome_canvas_text_realize (GnomeCanvasItem *item);
static void gnome_canvas_text_unrealize (GnomeCanvasItem *item);
static void gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
				    int x, int y, int width, int height);
static double gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
				       GnomeCanvasItem **actual_item);
static void gnome_canvas_text_bounds (GnomeCanvasItem *item,
				      double *x1, double *y1, double *x2, double *y2);
static void gnome_canvas_text_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static void gnome_canvas_text_set_markup (GnomeCanvasText *textitem,
					  const gchar     *markup);

static void gnome_canvas_text_set_font_desc    (GnomeCanvasText *textitem,
					        PangoFontDescription *font_desc);

static void gnome_canvas_text_apply_font_desc  (GnomeCanvasText *textitem);
static void gnome_canvas_text_apply_attributes (GnomeCanvasText *textitem);

static void add_attr (PangoAttrList  *attr_list,
		      PangoAttribute *attr);

static GnomeCanvasItemClass *parent_class;



/**
 * gnome_canvas_text_get_type:
 * @void: 
 * 
 * Registers the &GnomeCanvasText class if necessary, and returns the type ID
 * associated to it.
 * 
 * Return value: The type ID of the &GnomeCanvasText class.
 **/
GtkType
gnome_canvas_text_get_type (void)
{
	static GtkType text_type = 0;

	if (!text_type) {
		GtkTypeInfo text_info = {
			"GnomeCanvasText",
			sizeof (GnomeCanvasText),
			sizeof (GnomeCanvasTextClass),
			(GtkClassInitFunc) gnome_canvas_text_class_init,
			(GtkObjectInitFunc) gnome_canvas_text_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		text_type = gtk_type_unique (gnome_canvas_item_get_type (), &text_info);
	}

	return text_type;
}

/* Class initialization function for the text item */
static void
gnome_canvas_text_class_init (GnomeCanvasTextClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gobject_class->set_property = gnome_canvas_text_set_property;
	gobject_class->get_property = gnome_canvas_text_get_property;

	/* Text */
        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT,
                 g_param_spec_string ("text",
				      _("Text"),
				      _("Text to render"),
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

        g_object_class_install_property
                (gobject_class,
                 PROP_MARKUP,
                 g_param_spec_string ("markup",
				      _("Markup"),
				      _("Marked up text to render"),
				      NULL,
                                      (G_PARAM_WRITABLE)));

	/* Position */
        g_object_class_install_property
                (gobject_class,
                 PROP_X,
                 g_param_spec_double ("x", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

        g_object_class_install_property
                (gobject_class,
                 PROP_Y,
                 g_param_spec_double ("y", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));


	/* Font */
	g_object_class_install_property
                (gobject_class,
                 PROP_FONT,
                 g_param_spec_string ("font",
				      _("Font"),
				      _("Font description as a string"),
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
        g_object_class_install_property
		(gobject_class,
		 PROP_FONT_DESC,
		 g_param_spec_boxed ("font_desc",
				     _("Font description"),
				     _("Font description as a PangoFontDescription struct"),
				     GTK_TYPE_PANGO_FONT_DESCRIPTION,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	g_object_class_install_property
		(gobject_class,
		 PROP_FAMILY,
		 g_param_spec_string ("family",
				      _("Font family"),
				      _("Name of the font family, e.g. Sans, Helvetica, Times, Monospace"),
				      NULL,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
	/* Style */
        g_object_class_install_property
                (gobject_class,
                 PROP_ATTRIBUTES,
                 g_param_spec_boxed ("attributes", NULL, NULL,
				     PANGO_TYPE_ATTR_LIST,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_STYLE,
		 g_param_spec_enum ("style",
				    _("Font style"),
				    _("Font style"),
				    PANGO_TYPE_STYLE,
				    PANGO_STYLE_NORMAL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_VARIANT,
		 g_param_spec_enum ("variant",
				    _("Font variant"),
				    _("Font variant"),
				    PANGO_TYPE_VARIANT,
				    PANGO_VARIANT_NORMAL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_WEIGHT,
		 g_param_spec_int ("weight",
				   _("Font weight"),
				   _("Font weight"),
				   0,
				   G_MAXINT,
				   PANGO_WEIGHT_NORMAL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	
	g_object_class_install_property
		(gobject_class,
		 PROP_STRETCH,
		 g_param_spec_enum ("stretch",
				    _("Font stretch"),
				    _("Font stretch"),
				    PANGO_TYPE_STRETCH,
				    PANGO_STRETCH_NORMAL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_SIZE,
		 g_param_spec_int ("size",
				   _("Font size"),
				   _("Font size"),
				   0,
				   G_MAXINT,
				   0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		PROP_SIZE_POINTS,
		g_param_spec_double ("size_points",
				     _("Font points"),
				     _("Font size in points"),
				     0.0,
				     G_MAXDOUBLE,
				     0.0,
				     G_PARAM_READABLE | G_PARAM_WRITABLE));  
	
	g_object_class_install_property
		(gobject_class,
		 PROP_RISE,
		 g_param_spec_int ("rise",
				   _("Rise"),
				   _("Offset of text above the baseline (below the baseline if rise is negative)"),
				   -G_MAXINT,
				   G_MAXINT,
				   0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_STRIKETHROUGH,
		 g_param_spec_boolean ("strikethrough",
				       _("Strikethrough"),
				       _("Whether to strike through the text"),
				       FALSE,
				       G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property
		(gobject_class,
		 PROP_UNDERLINE,
		 g_param_spec_enum ("underline",
				    _("Underline"),
				    _("Style of underline for this text"),
				    PANGO_TYPE_UNDERLINE,
				    PANGO_UNDERLINE_NONE,
				    G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property
		(gobject_class,
		 PROP_SCALE,
		 g_param_spec_double ("scale",
				      _("Scale"),
				      _("Size of font, relative to default size"),
				      0.0,
				      G_MAXDOUBLE,
				      1.0,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));  
	
        g_object_class_install_property
		(gobject_class,
                 PROP_ANCHOR,
                 g_param_spec_enum ("anchor", NULL, NULL,
                                    GTK_TYPE_ANCHOR_TYPE,
                                    GTK_ANCHOR_NW,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_JUSTIFICATION,
                 g_param_spec_enum ("justification", NULL, NULL,
                                    GTK_TYPE_JUSTIFICATION,
                                    GTK_JUSTIFY_LEFT,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP_WIDTH,
                 g_param_spec_double ("clip_width", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP_HEIGHT,
                 g_param_spec_double ("clip_height", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP,
                 g_param_spec_boolean ("clip", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X_OFFSET,
                 g_param_spec_double ("x_offset", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y_OFFSET,
                 g_param_spec_double ("y_offset", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill_color",
				      _("Color"),
				      _("Text color, as string"),
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_GDK,
                 g_param_spec_boxed ("fill_color_gdk",
				     _("Color"),
				     _("Text color, as a GdkColor"),
				     GDK_TYPE_COLOR,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill_color_rgba",
				    _("Color"),
				    _("Text color, as an R/G/B/A combined integer"),
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_STIPPLE,
                 g_param_spec_object ("fill_stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT_WIDTH,
                 g_param_spec_double ("text_width",
				      _("Text width"),
				      _("Width of the rendered text"),
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT_HEIGHT,
                 g_param_spec_double ("text_height",
				      _("Text height"),
				      _("Height of the rendered text"),
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	/* Style props are set (explicitly applied) or not */
#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (gobject_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE))

	ADD_SET_PROP ("family_set", PROP_FAMILY_SET,
		      _("Font family set"),
		      _("Whether this tag affects the font family"));  
	
	ADD_SET_PROP ("style_set", PROP_STYLE_SET,
		      _("Font style set"),
		      _("Whether this tag affects the font style"));
	
	ADD_SET_PROP ("variant_set", PROP_VARIANT_SET,
		      _("Font variant set"),
		      _("Whether this tag affects the font variant"));
	
	ADD_SET_PROP ("weight_set", PROP_WEIGHT_SET,
		      _("Font weight set"),
		      _("Whether this tag affects the font weight"));
	
	ADD_SET_PROP ("stretch_set", PROP_STRETCH_SET,
		      _("Font stretch set"),
		      _("Whether this tag affects the font stretch"));
	
	ADD_SET_PROP ("size_set", PROP_SIZE_SET,
		      _("Font size set"),
		      _("Whether this tag affects the font size"));
	
	ADD_SET_PROP ("rise_set", PROP_RISE_SET,
		      _("Rise set"),
		      _("Whether this tag affects the rise"));
	
	ADD_SET_PROP ("strikethrough_set", PROP_STRIKETHROUGH_SET,
		      _("Strikethrough set"),
		      _("Whether this tag affects strikethrough"));
	
	ADD_SET_PROP ("underline_set", PROP_UNDERLINE_SET,
		      _("Underline set"),
		      _("Whether this tag affects underlining"));

	ADD_SET_PROP ("scale_set", PROP_SCALE_SET,
		      _("Scale set"),
		      _("Whether this tag affects font scaling"));
#undef ADD_SET_PROP
	
	object_class->destroy = gnome_canvas_text_destroy;

	item_class->update = gnome_canvas_text_update;
	item_class->realize = gnome_canvas_text_realize;
	item_class->unrealize = gnome_canvas_text_unrealize;
	item_class->draw = gnome_canvas_text_draw;
	item_class->point = gnome_canvas_text_point;
	item_class->bounds = gnome_canvas_text_bounds;
	item_class->render = gnome_canvas_text_render;
}

/* Object initialization function for the text item */
static void
gnome_canvas_text_init (GnomeCanvasText *text)
{
	text->x = 0.0;
	text->y = 0.0;
	text->anchor = GTK_ANCHOR_CENTER;
	text->justification = GTK_JUSTIFY_LEFT;
	text->clip_width = 0.0;
	text->clip_height = 0.0;
	text->xofs = 0.0;
	text->yofs = 0.0;
	text->layout = NULL;

	text->font_desc.family_name = NULL;
	
	text->family_set  = FALSE;
	text->style_set   = FALSE;
	text->variant_set = FALSE;
	text->weight_set  = FALSE;
	text->stretch_set = FALSE;
	text->size_set    = FALSE;

	text->underline     = PANGO_UNDERLINE_NONE;
	text->strikethrough = FALSE;
	text->rise          = 0;
	
	text->underline_set = FALSE;
	text->strike_set    = FALSE;
	text->rise_set      = FALSE;
	
	text->private = g_new(GnomeCanvasTextPrivate, 1);
	text->private->bitmap.buffer = NULL;
	text->private->render_dirty = 1;
}

/* Destroy handler for the text item */
static void
gnome_canvas_text_destroy (GtkObject *object)
{
	GnomeCanvasText *text;

	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	/* remember, destroy can be run multiple times! */

	g_free (text->text);
	text->text = NULL;

	if (text->layout)
	    g_object_unref (G_OBJECT (text->layout));
	text->layout = NULL;
	
	if (text->font_desc.family_name)
		g_free (text->font_desc.family_name);
	text->font_desc.family_name = NULL;

	if (text->attr_list)
		pango_attr_list_unref (text->attr_list);
	text->attr_list = NULL;
	
	if (text->stipple)
		gdk_bitmap_unref (text->stipple);
	text->stipple = NULL;

	if (text->private && text->private->bitmap.buffer) {
		g_free (text->private->bitmap.buffer);		
	}
	g_free (text->private);
	text->private = NULL;
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
get_bounds_item_relative (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double x, y;
	double clip_x, clip_y;

	item = GNOME_CANVAS_ITEM (text);

	x = text->x;
	y = text->y;

	clip_x = x;
	clip_y = y;

	/* Calculate text dimensions */

	if (text->layout)
	        pango_layout_get_pixel_size (text->layout,
					     &text->max_width,
					     &text->height);
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->max_width / 2;
		clip_x -= text->clip_width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->max_width;
		clip_x -= text->clip_width;
		break;

	default:
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		clip_y -= text->clip_height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		clip_y -= text->clip_height;
		break;

	default:
		break;
	}

	/* Bounds */


	

	if (text->clip) {
		/* maybe do bbox intersection here? */
		*px1 = clip_x;
		*py1 = clip_y;
		*px2 = clip_x + text->clip_width;
		*py2 = clip_y + text->clip_height;
	} else {
		*px1 = x;
		*py1 = y;
		*px2 = x + text->max_width;
		*py2 = y + text->height;
	}
}

static void
get_bounds (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (text);

	/* Get canvas pixel coordinates for text position */

	
	wx = text->x;
	wy = text->y;
	gnome_canvas_item_i2w (item, &wx, &wy);
	gnome_canvas_w2c (item->canvas, wx + text->xofs, wy + text->yofs, &text->cx, &text->cy);

	/* Get canvas pixel coordinates for clip rectangle position */

	gnome_canvas_w2c (item->canvas, wx, wy, &text->clip_cx, &text->clip_cy);
	text->clip_cwidth = text->clip_width * item->canvas->pixels_per_unit;
	text->clip_cheight = text->clip_height * item->canvas->pixels_per_unit;

	/* Calculate text dimensions */

	if (text->layout)
	        pango_layout_get_pixel_size (text->layout,
					     &text->max_width,
					     &text->height);
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		text->cx -= text->max_width / 2;
		text->clip_cx -= text->clip_cwidth / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		text->cx -= text->max_width;
		text->clip_cx -= text->clip_cwidth;
		break;

	default:
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		text->cy -= text->height / 2;
		text->clip_cy -= text->clip_cheight / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		text->cy -= text->height;
		text->clip_cy -= text->clip_cheight;
		break;

	default:
		break;
	}

	/* Bounds */

	if (text->clip) {
		*px1 = text->clip_cx;
		*py1 = text->clip_cy;
		*px2 = text->clip_cx + text->clip_cwidth;
		*py2 = text->clip_cy + text->clip_cheight;
	} else {
		*px1 = text->cx;
		*py1 = text->cy;
		*px2 = text->cx + text->max_width;
		*py2 = text->cy + text->height;
	}
}

/* Recalculates the bounding box of the text item.  The bounding box is defined
 * by the text's extents if the clip rectangle is disabled.  If it is enabled,
 * the bounding box is defined by the clip rectangle itself.
 */
static void
recalc_bounds (GnomeCanvasText *text)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (text);

	get_bounds (text, &item->x1, &item->y1, &item->x2, &item->y2);

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

/* Convenience function to set the text's GC's foreground color */
static void
set_text_gc_foreground (GnomeCanvasText *text)
{
	GdkColor c;

	if (!text->gc)
		return;

	c.pixel = text->pixel;
	gdk_gc_set_foreground (text->gc, &c);
}

/* Sets the stipple pattern for the text */
static void
set_stipple (GnomeCanvasText *text, GdkBitmap *stipple, int reconfigure)
{
	if (text->stipple && !reconfigure)
		gdk_bitmap_unref (text->stipple);

	text->stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (text->gc) {
		if (stipple) {
			gdk_gc_set_stipple (text->gc, stipple);
			gdk_gc_set_fill (text->gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (text->gc, GDK_SOLID);
	}
}

/* Set_arg handler for the text item */
static void
gnome_canvas_text_set_property (GObject            *object,
				guint               param_id,
				const GValue       *value,
				GParamSpec         *pspec)
{
	GnomeCanvasItem *item;
	GnomeCanvasText *text;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	gboolean color_changed;
	int have_pixel;
	PangoAlignment align;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);

	color_changed = FALSE;
	have_pixel = FALSE;
	

	if (!text->layout) {
	        PangoContext *gtk_context, *context;
		gtk_context = gtk_widget_get_pango_context (GTK_WIDGET (item->canvas));

		
	        if (item->canvas->aa)  {
			PangoLanguage *language;
		        context = pango_ft2_get_context ();
			language = pango_context_get_language (gtk_context);
			pango_context_set_language (context, language);
			pango_context_set_base_dir (context,
						    pango_context_get_base_dir (gtk_context));
		} else
			context = gtk_context;
			

		text->layout = pango_layout_new (context);
		
	        if (item->canvas->aa)
		        g_object_unref (G_OBJECT (context));
	}

	switch (param_id) {
	case PROP_TEXT:
		if (text->text)
			g_free (text->text);

		text->text = g_value_dup_string (value);
		pango_layout_set_text (text->layout, text->text, -1);
		recalc_bounds (text);

		text->private->render_dirty = 1;
		break;

	case PROP_MARKUP:
		gnome_canvas_text_set_markup (text,
					      g_value_get_string (value));
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_X:
		text->x = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_Y:
		text->y = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_FONT: {
		const char *font_name;

		font_name = g_value_get_string (value);
		if (font_name) {
			PangoFontDescription *font_desc;

			font_desc = pango_font_description_from_string (font_name);
			gnome_canvas_text_set_font_desc (text, font_desc);
			pango_font_description_free (font_desc);
			
			gnome_canvas_text_apply_font_desc (text);
			recalc_bounds (text);
		} else {
			text->family_set  = FALSE;
			text->style_set   = FALSE;
			text->variant_set = FALSE;
			text->weight_set  = FALSE;
			text->stretch_set = FALSE;
			text->size_set    = FALSE;

			gnome_canvas_text_apply_font_desc (text);
		}
		text->private->render_dirty = 1;
		break;
	}

	case PROP_FONT_DESC:
		gnome_canvas_text_set_font_desc (
			text, g_value_peek_pointer (value));
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_FAMILY:
		text->font_desc.family_name = g_strdup (g_value_get_string (value));
		text->family_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_FAMILY_SET:
		text->family_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;
		
	case PROP_STYLE:
		text->font_desc.style = g_value_get_enum (value);
		text->style_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_STYLE_SET:
		text->style_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;
		
	case PROP_VARIANT:
		text->font_desc.variant = g_value_get_enum (value);
		text->variant_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_VARIANT_SET:
		text->variant_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;
		
	case PROP_WEIGHT:
		text->font_desc.weight = g_value_get_int (value);
		text->weight_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_WEIGHT_SET:
		text->weight_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_STRETCH:
		text->font_desc.stretch = g_value_get_enum (value);
		text->stretch_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_STRETCH_SET:
		text->stretch_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_SIZE:
		text->font_desc.size = g_value_get_int (value);
		text->size_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_SIZE_POINTS:
		text->font_desc.size = g_value_get_double (value) * PANGO_SCALE;
		text->size_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_SIZE_SET:
		text->size_set = g_value_get_boolean (value);

		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_SCALE:
		text->scale = g_value_get_double (value);
		text->scale_set = TRUE;
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;
		
	case PROP_SCALE_SET:
		text->scale_set = g_value_get_boolean (value);
		
		gnome_canvas_text_apply_font_desc (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;		
		
	case PROP_UNDERLINE:
		text->underline = g_value_get_enum (value);
		text->underline_set = TRUE;
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_UNDERLINE_SET:
		text->underline_set = g_value_get_boolean (value);
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_STRIKETHROUGH:
		text->strikethrough = g_value_get_boolean (value);
		text->strike_set = TRUE;
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_STRIKETHROUGH_SET:
		text->strike_set = g_value_get_boolean (value);
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_RISE:
		text->rise = g_value_get_int (value);
		text->rise_set = TRUE;
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_RISE_SET:
		text->rise_set = TRUE;
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_ATTRIBUTES:
		if (text->attr_list)
			pango_attr_list_unref (text->attr_list);

		text->attr_list = g_value_peek_pointer (value);
		
		gnome_canvas_text_apply_attributes (text);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_ANCHOR:
		text->anchor = g_value_get_enum (value);
		recalc_bounds (text);
		break;

	case PROP_JUSTIFICATION:
		text->justification = g_value_get_enum (value);

		switch (text->justification) {
		case GTK_JUSTIFY_LEFT:
		        align = PANGO_ALIGN_LEFT;
			break;
		case GTK_JUSTIFY_CENTER:
		        align = PANGO_ALIGN_CENTER;
			break;
		case GTK_JUSTIFY_RIGHT:
		        align = PANGO_ALIGN_RIGHT;
			break;
		default:
		        /* GTK_JUSTIFY_FILL isn't supported yet. */
		        align = PANGO_ALIGN_LEFT;
			break;
		}		  
		pango_layout_set_alignment (text->layout, align);
		recalc_bounds (text);
		text->private->render_dirty = 1;				
		break;

	case PROP_CLIP_WIDTH:
		text->clip_width = fabs (g_value_get_double (value));
		recalc_bounds (text);
		text->private->render_dirty = 1;				
		break;

	case PROP_CLIP_HEIGHT:
		text->clip_height = fabs (g_value_get_double (value));
		recalc_bounds (text);
		text->private->render_dirty = 1;				
		break;

	case PROP_CLIP:
		text->clip = g_value_get_boolean (value);
		recalc_bounds (text);
		text->private->render_dirty = 1;
		break;

	case PROP_X_OFFSET:
		text->xofs = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_Y_OFFSET:
		text->yofs = g_value_get_double (value);
		recalc_bounds (text);
		break;

        case PROP_FILL_COLOR: {
		const char *color_name;

		color_name = g_value_get_string (value);
		if (color_name) {
			gdk_color_parse (color_name, &color);

			text->rgba = ((color.red & 0xff00) << 16 |
				      (color.green & 0xff00) << 8 |
				      (color.blue & 0xff00) |
				      0xff);
			color_changed = TRUE;
		}
		text->private->render_dirty = 1;
		break;
	}

	case PROP_FILL_COLOR_GDK:
		pcolor = g_value_get_boxed (value);
		if (pcolor) {
		    GdkColormap *colormap;

		    color = *pcolor;
		    colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
		    gdk_rgb_find_color (colormap, &color);
		    have_pixel = TRUE;
		}

		text->rgba = ((color.red & 0xff00) << 16 |
			      (color.green & 0xff00) << 8|
			      (color.blue & 0xff00) |
			      0xff);
		color_changed = TRUE;
		break;

        case PROP_FILL_COLOR_RGBA:
		text->rgba = g_value_get_uint (value);
		color_changed = TRUE;
		text->private->render_dirty = 1;
		break;

	case PROP_FILL_STIPPLE:
		set_stipple (text, (GdkBitmap *)g_value_get_object (value), FALSE);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (color_changed) {
		if (have_pixel)
			text->pixel = color.pixel;
		else
			text->pixel = gnome_canvas_get_color_pixel (item->canvas, text->rgba);

		if (!item->canvas->aa)
			set_text_gc_foreground (text);

		gnome_canvas_item_request_update (item);
	}
}

/* Get_arg handler for the text item */
static void
gnome_canvas_text_get_property (GObject            *object,
				guint               param_id,
				GValue             *value,
				GParamSpec         *pspec)
{
	GnomeCanvasText *text;
	GdkColor *color;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	switch (param_id) {
	case PROP_TEXT:
		g_value_set_string (value, text->text);
		break;

	case PROP_X:
		g_value_set_double (value, text->x);
		break;

	case PROP_Y:
		g_value_set_double (value, text->y);
		break;

	case PROP_FONT_DESC:
		g_value_set_boxed (value, &(text->font_desc));
		break;

	case PROP_FAMILY:
		g_value_set_string (value, text->font_desc.family_name);
		break;
	case PROP_FAMILY_SET:
		g_value_set_boolean (value, text->family_set);
		break;
		
	case PROP_STYLE:
		g_value_set_enum (value, text->font_desc.style);
		break;
	case PROP_STYLE_SET:
		g_value_set_boolean (value, text->style_set);
		break;
		
	case PROP_VARIANT:
		g_value_set_enum (value, text->font_desc.variant);
		break;
	case PROP_VARIANT_SET:
		g_value_set_boolean (value, text->variant_set);
		break;

	case PROP_WEIGHT:
		g_value_set_int (value, text->font_desc.weight);
		break;
	case PROP_WEIGHT_SET:
		g_value_set_boolean (value, text->weight_set);
		break;
		
	case PROP_STRETCH:
		g_value_set_enum (value, text->font_desc.stretch);
		break;
	case PROP_STRETCH_SET:
		g_value_set_boolean (value, text->stretch_set);
		break;
		
	case PROP_SIZE:
		g_value_set_int (value, text->font_desc.size);
		break;
	case PROP_SIZE_POINTS:
		g_value_set_double (value, ((double)text->font_desc.size) / (double)PANGO_SCALE);
		break;
	case PROP_SIZE_SET:
		g_value_set_boolean (value, text->size_set);
		break;
		
	case PROP_SCALE:
		g_value_set_double (value, text->scale);
		break;
	case PROP_SCALE_SET:
		g_value_set_boolean (value, text->scale_set);
		break;
		
	case PROP_UNDERLINE:
		g_value_set_enum (value, text->underline);
		break;
	case PROP_UNDERLINE_SET:
		g_value_set_boolean (value, text->underline_set);
		break;
		
	case PROP_STRIKETHROUGH:
		g_value_set_boolean (value, text->strikethrough);
		break;
	case PROP_STRIKETHROUGH_SET:
		g_value_set_boolean (value, text->strike_set);
		break;
		
	case PROP_RISE:
		g_value_set_int (value, text->rise);
		break;
	case PROP_RISE_SET:
		g_value_set_boolean (value, text->rise_set);
		break;
		
	case PROP_ATTRIBUTES:
		g_value_set_boxed (value, text->attr_list);
		break;

	case PROP_ANCHOR:
		g_value_set_enum (value, text->anchor);
		break;

	case PROP_JUSTIFICATION:
		g_value_set_enum (value, text->justification);
		break;

	case PROP_CLIP_WIDTH:
		g_value_set_enum (value, text->clip_width);
		break;

	case PROP_CLIP_HEIGHT:
		g_value_set_double (value, text->clip_height);
		break;

	case PROP_CLIP:
		g_value_set_boolean (value, text->clip);
		break;

	case PROP_X_OFFSET:
		g_value_set_double (value, text->xofs);
		break;

	case PROP_Y_OFFSET:
		g_value_set_double (value, text->yofs);
		break;

	case PROP_FILL_COLOR_GDK: {
		GdkColormap *colormap;

		color = g_new (GdkColor, 1);
		color->pixel = text->pixel;
		colormap = gtk_widget_get_colormap (GTK_WIDGET (text->item.canvas));
		gdk_rgb_find_color (colormap, color);
		g_value_set_boxed (value, color);
		break;
	}
	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, text->rgba);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_boxed (value, text->stipple);
		break;

	case PROP_TEXT_WIDTH:
		g_value_set_double (value, text->max_width / text->item.canvas->pixels_per_unit);
		break;

	case PROP_TEXT_HEIGHT:
		g_value_set_double (value, text->height / text->item.canvas->pixels_per_unit);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* */
static void
gnome_canvas_text_apply_font_desc (GnomeCanvasText *text)
{
	PangoFontDescription *font_desc =
		pango_font_description_copy (
			GTK_WIDGET (GNOME_CANVAS_ITEM (text)->canvas)->style->font_desc);
	
	if (text->family_set && text->font_desc.family_name)
	{
		if (font_desc->family_name)
			g_free (font_desc->family_name);
		font_desc->family_name = g_strdup (text->font_desc.family_name);
	}
		
	if (text->style_set)
		font_desc->style = text->font_desc.style;

	if (text->variant_set)
		font_desc->variant = text->font_desc.variant;

	if (text->weight_set)
		font_desc->weight = text->font_desc.weight;
	
	if (text->stretch_set)
		font_desc->stretch = text->font_desc.stretch;

	if (text->size_set)
		font_desc->size = text->font_desc.size;

	if (text->scale_set)
		font_desc->size *= text->scale;
	
	pango_layout_set_font_description (text->layout,
					   font_desc);
	pango_font_description_free (font_desc);
}

static void
add_attr (PangoAttrList  *attr_list,
	  PangoAttribute *attr)
{
	attr->start_index = 0;
	attr->end_index = G_MAXINT;

	pango_attr_list_insert (attr_list, attr);
}

/* */
static void
gnome_canvas_text_apply_attributes (GnomeCanvasText *text)
{
	PangoAttrList *attr_list;

	if (text->attr_list)
		attr_list = pango_attr_list_copy (text->attr_list);
	else
		attr_list = pango_attr_list_new ();
	
	if (text->underline_set)
		add_attr (attr_list, pango_attr_underline_new (text->underline));
	if (text->strike_set)
		add_attr (attr_list, pango_attr_strikethrough_new (text->strikethrough));
	if (text->rise_set)
		add_attr (attr_list, pango_attr_rise_new (text->rise));
	
	pango_layout_set_attributes (text->layout, attr_list);
	pango_attr_list_unref (attr_list);
}

static void
gnome_canvas_text_set_font_desc (GnomeCanvasText      *text,
				 PangoFontDescription *font_desc)
{
	if (text->font_desc.family_name)
		g_free (text->font_desc.family_name);

	text->font_desc.family_name = g_strdup (font_desc->family_name);

	text->font_desc.style   = font_desc->style;
	text->font_desc.variant = font_desc->variant;
	text->font_desc.weight  = font_desc->weight;
	text->font_desc.stretch = font_desc->stretch;
	text->font_desc.size    = font_desc->size;
	
	text->family_set  = TRUE;
	text->style_set   = TRUE;
	text->variant_set = TRUE;
	text->weight_set  = TRUE;
	text->stretch_set = TRUE;
	text->size_set    = TRUE;
}

/* Setting the text from a Pango markup string */
static void
gnome_canvas_text_set_markup (GnomeCanvasText *textitem,
			      const gchar     *markup)
{
	PangoAttrList *attr_list = NULL;
	gchar         *text = NULL;
	GError        *error = NULL;

	if (textitem->text)
		g_free (textitem->text);
	if (textitem->attr_list)
		pango_attr_list_unref (textitem->attr_list);

	if (markup && !pango_parse_markup (markup, -1,
					   0,
					   &attr_list, &text, NULL,
					   &error))
	{
		g_warning ("Failed to set cell text from markup due to error parsing markup: %s",
			   error->message);
		g_error_free (error);
		return;
	}

	textitem->text = text;
	textitem->attr_list = attr_list;

	pango_layout_set_text (textitem->layout, text, -1);

	gnome_canvas_text_apply_attributes (textitem);
}

/* Update handler for the text item */
static void
gnome_canvas_text_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasText *text;
	double x1, y1, x2, y2;
	ArtDRect i_bbox, c_bbox;
	int i;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (!item->canvas->aa) {
		set_text_gc_foreground (text);
		set_stipple (text, text->stipple, TRUE);
		get_bounds (text, &x1, &y1, &x2, &y2);

		gnome_canvas_update_bbox (item, x1, y1, x2, y2);
	} else {
		/* aa rendering */
		for (i = 0; i < 6; i++)
			text->affine[i] = affine[i];
		get_bounds_item_relative (text, &i_bbox.x0, &i_bbox.y0, &i_bbox.x1, &i_bbox.y1);
		art_drect_affine_transform (&c_bbox, &i_bbox, affine);

		
		gnome_canvas_update_bbox (item, floor(c_bbox.x0), floor(c_bbox.y0), ceil(c_bbox.x1), ceil(c_bbox.y1));

	}
}

/* Realize handler for the text item */
static void
gnome_canvas_text_realize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	text->gc = gdk_gc_new (item->canvas->layout.bin_window);
}

/* Unrealize handler for the text item */
static void
gnome_canvas_text_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	gdk_gc_unref (text->gc);
	text->gc = NULL;

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

/* Draw handler for the text item */
static void
gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasText *text;
	GdkRectangle rect;

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text)
		return;

	if (text->clip) {
		rect.x = text->clip_cx - x;
		rect.y = text->clip_cy - y;
		rect.width = text->clip_cwidth;
		rect.height = text->clip_cheight;

		gdk_gc_set_clip_rectangle (text->gc, &rect);
	}

	if (text->stipple)
		gnome_canvas_set_stipple_origin (item->canvas, text->gc);


	gdk_draw_layout (drawable, text->gc, text->cx - x, text->cy - y, text->layout);

	if (text->clip)
		gdk_gc_set_clip_rectangle (text->gc, NULL);
}


/* Render handler for the text item */
static void
gnome_canvas_text_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	GnomeCanvasText *text;
	guint32 fg_color;
	double affine[6], anchor_affine[6], anchor_x = 0.0, anchor_y = 0.0,
		bitmap_width, bitmap_height;


	int render_x = 0, render_y = 0; /* offsets for text rendering,
					 * for clipping rectangles */


	
	text = GNOME_CANVAS_TEXT (item);

	if (!text->text)
		return;

	fg_color = text->rgba >> 8;

        gnome_canvas_buf_ensure_buf (buf);

	
	if(text->private->render_dirty) {		
		if(text->private->bitmap.buffer) {
			g_free(text->private->bitmap.buffer);
		}
		text->private->bitmap.rows =  (text->clip) ? text->clip_height : text->height;
		text->private->bitmap.width = (text->clip) ? text->clip_width : text->max_width;
		text->private->bitmap.pitch = (text->max_width+3)&~3;
		text->private->bitmap.buffer = g_malloc0 (text->private->bitmap.rows * text->private->bitmap.pitch);
		text->private->bitmap.num_grays = 256;
		text->private->bitmap.pixel_mode = ft_pixel_mode_grays;


		/* What this does is when a clipping rectangle is
		   being used shift the rendering of the text by the
		   correct amount so that the correct result is
		   obtained as if all text was rendered, then clipped.
		   In this sense we can use smaller buffers and less
		   rendeirng since hopefully FreeType2 checks to see
		   if the glyph falls in the bounding box before
		   rasterizing it. */

		if(text->clip) {
			switch(text->anchor) {
			case GTK_ANCHOR_NW:				
				break;
			case GTK_ANCHOR_W:
				render_y -= (text->height-text->clip_height)*0.5;
				break;
			case GTK_ANCHOR_SW:
				/* you don't want this */
				render_y -= (text->height-text->clip_height);
				break;
			case GTK_ANCHOR_N:
				render_x -= (text->max_width-text->clip_width)*0.5;
				break;
			case GTK_ANCHOR_CENTER:
				render_x -= (text->max_width-text->clip_width)*0.5;
				render_y -= (text->height-text->clip_height)*0.5;
				break;
			case GTK_ANCHOR_S:
				render_x -= (text->max_width-text->clip_width)*0.5;
				render_y -= (text->height-text->clip_height);
				break;
			case GTK_ANCHOR_NE:
				render_x -= (text->max_width-text->clip_width);
				break;
			case GTK_ANCHOR_E:
				render_x -= (text->max_width-text->clip_width);
				render_y -= (text->height-text->clip_height)*0.5;
				break;
			case GTK_ANCHOR_SE:
			        render_x -= (text->max_width-text->clip_width);
				render_y -= (text->height-text->clip_height);
			default:
				break;			
			}
		}
		

		pango_ft2_render_layout (&text->private->bitmap, text->layout, render_x, render_y);
		
		text->private->render_dirty = 0;
	}



	memcpy(affine, text->affine, sizeof(gdouble)*6);       

	anchor_x = text->x*item->canvas->pixels_per_unit;
	anchor_y = text->y*item->canvas->pixels_per_unit;

	bitmap_width = text->private->bitmap.width;
	bitmap_height = text->private->bitmap.rows;

	
	/* Figure out the correct translation to apply to affine due
	 * to the anchor */
	switch(text->anchor) {
	case GTK_ANCHOR_NW:
		break;
	case GTK_ANCHOR_W:
		anchor_y -= (bitmap_height*0.5)*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_SW:
		anchor_y -= bitmap_height*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_N:
		anchor_x -= (bitmap_width*0.5)*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_CENTER:
		anchor_x -= (bitmap_width*0.5)*item->canvas->pixels_per_unit;
		anchor_y -= (bitmap_height*0.5)*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_S:
		anchor_x -= (bitmap_width*0.5)*item->canvas->pixels_per_unit;
		anchor_y -= bitmap_height*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_NE:
		anchor_x -= (bitmap_width)*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_E:
		anchor_x -= bitmap_width*item->canvas->pixels_per_unit;
		anchor_y -= (bitmap_height*0.5)*item->canvas->pixels_per_unit;
		break;
	case GTK_ANCHOR_SE:
		anchor_x -= bitmap_width*item->canvas->pixels_per_unit;
		anchor_y -= bitmap_height*item->canvas->pixels_per_unit;
	default:
		break;
	}

	/* Honor the anchor offsets */
	anchor_x += text->xofs;
	anchor_y += text->yofs;
	
	art_affine_translate(anchor_affine, anchor_x, anchor_y);
	art_affine_multiply(affine, affine, anchor_affine);

	art_rgb_a_affine (buf->buf,
			  buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
			  buf->buf_rowstride,
			  text->private->bitmap.buffer,
			  text->private->bitmap.width,
			  text->private->bitmap.rows,
			  text->private->bitmap.pitch,
			  fg_color,
			  affine,
			  ART_FILTER_NEAREST, NULL);
			
	buf->is_bg = 0;
}

/* Point handler for the text item */
static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	PangoLayoutIter *iter;
	int x1, y1, x2, y2;
	int dx, dy;
	double dist, best;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	/* The idea is to build bounding rectangles for each of the lines of
	 * text (clipped by the clipping rectangle, if it is activated) and see
	 * whether the point is inside any of these.  If it is, we are done.
	 * Otherwise, calculate the distance to the nearest rectangle.
	 */

	best = 1.0e36;

	iter = pango_layout_get_iter (text->layout);
	do {
 	        PangoRectangle log_rect;

		pango_layout_iter_get_line_extents (iter, NULL, &log_rect);
				
		if (text->clip) {
			x1 = PANGO_PIXELS (log_rect.x);
			y1 = PANGO_PIXELS (log_rect.y);
			x2 = PANGO_PIXELS (log_rect.x+log_rect.width);
			y2 = PANGO_PIXELS (log_rect.x+log_rect.height);


			if (x1 < text->clip_cx)
				x1 = text->clip_cx;

			if (y1 < text->clip_cy)
				y1 = text->clip_cy;

			if (x2 > (text->clip_cx + text->clip_width))
				x2 = text->clip_cx + text->clip_width;

			if (y2 > (text->clip_cy + text->clip_height))
				y2 = text->clip_cy + text->clip_height;

			if ((x1 >= x2) || (y1 >= y2))
				continue;
		} else {
			x1 = text->x;
			y1 = text->y;
			x2 = log_rect.width;
			y2 = log_rect.height;			
		}

		/* Calculate distance from point to rectangle */

		if (cx < x1)
			dx = x1 - cx;
		else if (cx >= x2)
			dx = cx - x2 + 1;
		else
			dx = 0;

		if (cy < y1)
			dy = y1 - cy;
		else if (cy >= y2)
			dy = cy - y2 + 1;
		else
			dy = 0;

		if ((dx == 0) && (dy == 0))
			return 0.0;

		dist = sqrt (dx * dx + dy * dy);
		if (dist < best)
			best = dist;
		
	} while (pango_layout_iter_next_line(iter));

	return best / item->canvas->pixels_per_unit;
}

/* Bounds handler for the text item */
static void
gnome_canvas_text_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasText *text;
	double width, height;

	text = GNOME_CANVAS_TEXT (item);

	*x1 = text->x;
	*y1 = text->y;

	if (text->clip) {
		width = text->clip_width;
		height = text->clip_height;
	} else {
		width = text->max_width / item->canvas->pixels_per_unit;
		height = text->height / item->canvas->pixels_per_unit;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		*x1 -= width / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		*x1 -= width;
		break;

	default:
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		*y1 -= height / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		*y1 -= height;
		break;

	default:
		break;
	}

	*x2 = *x1 + width;
	*y2 = *y1 + height;	
}
