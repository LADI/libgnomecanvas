/* Bpath item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998,1999 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@acm.org>
 *          Lauris Kaplinski <lauris@ariman.ee>
 *          Miguel de Icaza <miguel@kernel.org>
 *          Cody Russell <bratsche@gnome.org>
 */

/* These includes are set up for standalone compile. If/when this codebase
   is integrated into libgnomeui, the includes will need to change. */

#include <math.h>
#include <string.h>

#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include "gnome-canvas.h"
#include "gnome-canvas-util.h"

#include "gnome-canvas-bpath.h"
#include "gnome-canvas-bpath-private.h"
#include "gp-path.h"

#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_vpath_dash.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_svp_point.h>

enum {
	PROP_0,
	PROP_BPATH,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_COLOR_GDK,
	PROP_OUTLINE_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_OUTLINE_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_WIDTH_UNITS,
	PROP_CAP_STYLE,
	PROP_JOIN_STYLE,
	PROP_WIND,
	PROP_MITERLIMIT,
	PROP_DASH
};

static void gnome_canvas_bpath_class_init   (GnomeCanvasBpathClass *class);
static void gnome_canvas_bpath_init         (GnomeCanvasBpath      *bpath);
static void gnome_canvas_bpath_destroy      (GtkObject               *object);
static void gnome_canvas_bpath_set_property (GObject               *object,
					     guint                  param_id,
					     const GValue          *value,
                                             GParamSpec            *pspec);
static void gnome_canvas_bpath_get_property (GObject               *object,
					     guint                  param_id,
					     GValue                *value,
                                             GParamSpec            *pspec);

static void   gnome_canvas_bpath_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void   gnome_canvas_bpath_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_bpath_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_bpath_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
                                              int x, int y, int width, int height);
static double gnome_canvas_bpath_point       (GnomeCanvasItem *item, double x, double y,
                                              int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_bpath_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);


static gulong get_pixel_from_rgba (GnomeCanvasItem *item, guint32 rgba_color);
static guint32 get_rgba_from_color (GdkColor * color);
static void set_gc_foreground (GdkGC *gc, gulong pixel);
static void gcbp_ensure_gdk (GnomeCanvasBpath * bpath);
static void gcbp_destroy_gdk (GnomeCanvasBpath * bpath);
static void set_stipple (GdkGC *gc, GdkBitmap **internal_stipple, GdkBitmap *stipple, int reconfigure);
static void gcbp_ensure_mask (GnomeCanvasBpath * bpath, gint width, gint height);
static void gcbp_draw_ctx_unref (GCBPDrawCtx * ctx);

static GnomeCanvasItemClass *parent_class;

GtkType
gnome_canvas_bpath_get_type (void)
{
	static GtkType bpath_type = 0;

	if (!bpath_type) {
		GtkTypeInfo bpath_info = {
			"GnomeCanvasBpath",
			sizeof (GnomeCanvasBpath),
			sizeof (GnomeCanvasBpathClass),
			(GtkClassInitFunc) gnome_canvas_bpath_class_init,
			(GtkObjectInitFunc) gnome_canvas_bpath_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		bpath_type = gtk_type_unique (gnome_canvas_item_get_type (), &bpath_info);
	}

	return bpath_type;
}

static void
gnome_canvas_bpath_class_init (GnomeCanvasBpathClass *class)
{
	GObjectClass         *gobject_class;
	GtkObjectClass       *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	/* when this gets checked into libgnomeui, change the
           GTK_TYPE_POINTER to GTK_TYPE_GNOME_CANVAS_BPATH, and add an
           entry to gnome-boxed.defs */

	g_object_class_install_property (gobject_class,
                                         PROP_BPATH,
                                         g_param_spec_pointer ("bpath", NULL, NULL,
                                                               (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_FILL_COLOR,
                                         g_param_spec_string ("fill_color", NULL, NULL,
                                                              NULL,
                                                              (G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_FILL_COLOR_GDK,
                                         g_param_spec_boxed ("fill_color_gdk", NULL, NULL,
                                                             GDK_TYPE_COLOR,
                                                             (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_FILL_COLOR_RGBA,
                                         g_param_spec_uint ("fill_color_rgba", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_OUTLINE_COLOR,
                                         g_param_spec_string ("outline_color", NULL, NULL,
                                                              NULL,
                                                              (G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_OUTLINE_COLOR_GDK,
                                         g_param_spec_boxed ("outline_color_gdk", NULL, NULL,
                                                             GDK_TYPE_COLOR,
                                                             (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_OUTLINE_COLOR_RGBA,
                                         g_param_spec_uint ("outline_color_rgba", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_FILL_STIPPLE,
                                         g_param_spec_object ("fill_stipple", NULL, NULL,
                                                              GDK_TYPE_DRAWABLE,
                                                              (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_OUTLINE_STIPPLE,
                                         g_param_spec_object ("outline_stipple", NULL, NULL,
                                                              GDK_TYPE_DRAWABLE,
                                                              (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_WIDTH_PIXELS,
                                         g_param_spec_uint ("width_pixels", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_WIDTH_UNITS,
                                         g_param_spec_double ("width_units", NULL, NULL,
                                                              0.0, G_MAXDOUBLE, 0.0,
                                                              (G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_CAP_STYLE,
                                         g_param_spec_enum ("cap_style", NULL, NULL,
                                                            GDK_TYPE_CAP_STYLE,
                                                            GDK_CAP_BUTT,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_JOIN_STYLE,
                                         g_param_spec_enum ("join_style", NULL, NULL,
                                                            GDK_TYPE_JOIN_STYLE,
                                                            GDK_JOIN_MITER,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_WIND,
                                         g_param_spec_uint ("wind", NULL, NULL,
                                                            0, G_MAXUINT, 0,
                                                            (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_MITERLIMIT,
                                         g_param_spec_double ("miterlimit", NULL, NULL,
                                                              0.0, G_MAXDOUBLE, 0.0,
                                                              (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property (gobject_class,
                                         PROP_DASH,
                                         g_param_spec_pointer ("dash", NULL, NULL,
                                                               (G_PARAM_READABLE | G_PARAM_WRITABLE)));

#if 0
	gtk_object_add_arg_type ("GnomeCanvasBpath::bpath", GTK_TYPE_POINTER, GTK_ARG_READWRITE, PROP_BPATH);
	gtk_object_add_arg_type ("GnomeCanvasBpath::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, PROP_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasBpath::fill_color_gdk", GDK_TYPE_COLOR, GTK_ARG_READWRITE, PROP_FILL_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasBpath::fill_color_rgba", GTK_TYPE_UINT, GTK_ARG_READWRITE, PROP_FILL_COLOR_RGBA);
	gtk_object_add_arg_type ("GnomeCanvasBpath::outline_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, PROP_OUTLINE_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasBpath::outline_color_gdk", GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, PROP_OUTLINE_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasBpath::outline_color_rgba", GTK_TYPE_UINT, GTK_ARG_READWRITE, PROP_OUTLINE_COLOR_RGBA);
	gtk_object_add_arg_type ("GnomeCanvasBpath::fill_stipple", GDK_TYPE_DRAWABLE, GTK_ARG_READWRITE, PROP_FILL_STIPPLE);
	gtk_object_add_arg_type ("GnomeCanvasBpath::outline_stipple", GTK_TYPE_GDK_WINDOW, GTK_ARG_READWRITE, PROP_OUTLINE_STIPPLE);
	gtk_object_add_arg_type ("GnomeCanvasBpath::width_pixels", GTK_TYPE_UINT, GTK_ARG_WRITABLE, PROP_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasBpath::width_units", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, PROP_WIDTH_UNITS);
	gtk_object_add_arg_type ("GnomeCanvasBpath::cap_style", GDK_TYPE_CAP_STYLE, GTK_ARG_READWRITE, PROP_CAP_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasBpath::join_style", GDK_TYPE_JOIN_STYLE, GTK_ARG_READWRITE, PROP_JOIN_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasBpath::wind", GTK_TYPE_ENUM, GTK_ARG_READWRITE, PROP_WIND);
	gtk_object_add_arg_type ("GnomeCanvasBpath::miterlimit", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, PROP_MITERLIMIT);
	gtk_object_add_arg_type ("GnomeCanvasBpath::dash", GTK_TYPE_POINTER, GTK_ARG_READWRITE, PROP_DASH);
#endif

	object_class->destroy = gnome_canvas_bpath_destroy;

	gobject_class->set_property = gnome_canvas_bpath_set_property;
	gobject_class->get_property = gnome_canvas_bpath_get_property;

	item_class->update = gnome_canvas_bpath_update;
	item_class->realize = gnome_canvas_bpath_realize;
	item_class->unrealize = gnome_canvas_bpath_unrealize;
	item_class->draw = gnome_canvas_bpath_draw;
	item_class->point = gnome_canvas_bpath_point;
	item_class->render = gnome_canvas_bpath_render;
}

static void
gnome_canvas_bpath_init (GnomeCanvasBpath *bpath)
{
	bpath->priv = g_new (GnomeCanvasBpathPriv, 1);

	bpath->priv->path = NULL;

	bpath->priv->fill_set = FALSE;
	bpath->priv->outline_set = FALSE;
	bpath->priv->width_pixels = FALSE;

	bpath->priv->width = 1.0;

	bpath->priv->fill_rgba = 0x0000003f;
	bpath->priv->outline_rgba = 0x0000007f;

	bpath->priv->cap = GDK_CAP_BUTT;
	bpath->priv->join = GDK_JOIN_MITER;
	bpath->priv->wind = ART_WIND_RULE_ODDEVEN;
	bpath->priv->miterlimit = 11.0;			/* PS default? */

	bpath->priv->dash.n_dash = 0;
	bpath->priv->dash.dash = NULL;

	bpath->priv->fill_svp = NULL;
	bpath->priv->outline_svp = NULL;

	bpath->priv->gdk = NULL;
}

static void
gnome_canvas_bpath_destroy (GtkObject *object)
{
	GnomeCanvasBpath *bpath;
	GnomeCanvasBpathPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_BPATH (object));

	bpath = GNOME_CANVAS_BPATH (object);

	if (bpath->priv) {
		priv = bpath->priv;
		if (priv->gdk) gcbp_destroy_gdk (bpath);

		if (priv->path) gp_path_unref (priv->path);

		if (priv->dash.dash) g_free (priv->dash.dash);
		if (priv->fill_svp) art_svp_free (priv->fill_svp);
		if (priv->outline_svp) art_svp_free (priv->outline_svp);
		
		g_free (bpath->priv);
		bpath->priv = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_canvas_bpath_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	GnomeCanvasItem         *item;
	GnomeCanvasBpath        *bpath;
	GnomeCanvasBpathPriv    *priv;
	GnomeCanvasBpathPrivGdk *gdk;
	GPPath                  *gpp;
	GdkColor                 color;
	GdkColor                *colorptr;
	ArtVpathDash            *dash;

	item = GNOME_CANVAS_ITEM (object);
	bpath = GNOME_CANVAS_BPATH (object);
	priv = bpath->priv;

	if (!item->canvas->aa) {
		gcbp_ensure_gdk (bpath);
		gdk = priv->gdk;
	} else {
		gdk = NULL;
	}

	switch (param_id) {
	case PROP_BPATH:
		gpp = g_value_get_pointer (value);

		if (priv->path) gp_path_unref (priv->path);

		if (gpp) {
			priv->path = gp_path_duplicate (gpp);
		} else {
			priv->path = NULL;
		}

		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR:
		if (gnome_canvas_get_color (item->canvas, g_value_get_string (value), &color)) {
			priv->fill_set = TRUE;
			priv->fill_rgba = get_rgba_from_color (&color);
			if (gdk) gdk->fill_pixel = color.pixel;
		} else {
			priv->fill_set = FALSE;
		}

		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR_GDK:
		colorptr = g_value_get_boxed (value);

		priv->fill_set = TRUE;
		priv->fill_rgba = get_rgba_from_color (colorptr);
		if (gdk) gdk->fill_pixel = colorptr->pixel;

		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR_RGBA:
		priv->fill_set = TRUE;
		priv->fill_rgba = g_value_get_uint (value);
		if (gdk) gdk->fill_pixel = get_pixel_from_rgba (item, priv->fill_rgba);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_OUTLINE_COLOR:
		if (gnome_canvas_get_color (item->canvas, g_value_get_string (value), &color)) {
			priv->outline_set = TRUE;
			priv->outline_rgba = get_rgba_from_color (&color);
			if (gdk) gdk->outline_pixel = color.pixel;
		} else {
			priv->outline_set = FALSE;
		}

		gnome_canvas_item_request_update (item);
		break;

	case PROP_OUTLINE_COLOR_GDK:
		colorptr = g_value_get_boxed (value);

		priv->outline_set = TRUE;
		priv->outline_rgba = get_rgba_from_color (colorptr);
		if (gdk) gdk->outline_pixel = colorptr->pixel;

		gnome_canvas_item_request_update (item);
		break;

	case PROP_OUTLINE_COLOR_RGBA:
		priv->outline_set = TRUE;
		priv->outline_rgba = g_value_get_uint (value);
		if (gdk) gdk->outline_pixel = get_pixel_from_rgba (item, priv->outline_rgba);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_STIPPLE:
		if (gdk) {
			set_stipple (gdk->fill_gc, &gdk->fill_stipple, g_value_get_boxed (value), FALSE);
			gnome_canvas_item_request_update (item);
		}
		break;

	case PROP_OUTLINE_STIPPLE:
		if (gdk) {
			set_stipple (gdk->outline_gc, &gdk->outline_stipple, g_value_get_boxed (value), FALSE);
			gnome_canvas_item_request_update (item);
		}
		break;

	case PROP_WIDTH_PIXELS:
		priv->width = g_value_get_uint (value);
		priv->width_pixels = TRUE;

		gnome_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_UNITS:
		priv->width = fabs (g_value_get_double (value));
		priv->width_pixels = FALSE;

		gnome_canvas_item_request_update (item);
		break;

	case PROP_WIND:
		priv->wind = g_value_get_enum (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_CAP_STYLE:
		priv->cap = g_value_get_enum (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_JOIN_STYLE:
		priv->join = g_value_get_enum (value);
		gnome_canvas_item_request_update (item);
		break;
	
	case PROP_MITERLIMIT:
		priv->miterlimit = g_value_get_double (value);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_DASH:
		dash = g_value_get_pointer (value);
		if (priv->dash.dash) g_free (priv->dash.dash);
		priv->dash.dash = NULL;

		if (dash) {
			priv->dash.offset = dash->offset;
			priv->dash.n_dash = dash->n_dash;
			if (dash->dash != NULL) {
				priv->dash.dash = g_new (double, dash->n_dash * sizeof (double));
				memcpy (priv->dash.dash, dash->dash, dash->n_dash * sizeof (double));
			}
		}
		gnome_canvas_item_request_update (item);
		break;

	default:
		break;
	}
}

/* Allocates a GdkColor structure filled with the specified pixel, and
 * puts it into the specified value for returning it in the get_property 
 * method.
 */

static void
get_color_value (GnomeCanvasBpath *bpath, gulong pixel, GValue *value)
{
	GdkColor *color;
	GdkColormap *colormap;

	color = g_new (GdkColor, 1);
	color->pixel = pixel;

	colormap = gtk_widget_get_colormap (GTK_WIDGET (bpath));
	gdk_rgb_find_color (colormap, color);
	g_value_set_boxed (value, color);
}

static void
gnome_canvas_bpath_get_property (GObject     *object,
                                 guint        param_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
	GnomeCanvasItem         *item;
	GnomeCanvasBpath        *bpath;
	GnomeCanvasBpathPriv    *priv;
	GnomeCanvasBpathPrivGdk *gdk;

	item = (GnomeCanvasItem *) object;
	bpath = (GnomeCanvasBpath *) object;
	priv = bpath->priv;

	if (!item->canvas->aa) {
		gcbp_ensure_gdk (bpath);
		gdk = priv->gdk;
	}
	else {
		gdk = NULL;
	}

	switch (param_id) {
	case PROP_BPATH:
		if (priv->path) {
			gp_path_ref (priv->path);
			g_value_set_pointer (value, priv->path);
		} else
			g_value_set_pointer (value, NULL);
		break;

	case PROP_FILL_COLOR_GDK:
		if (gdk) {
			get_color_value (bpath, gdk->fill_pixel, value);
		} else {
			get_color_value (bpath, 0, value);
		}
		break;
		
	case PROP_OUTLINE_COLOR_GDK:
		if (gdk) {
			get_color_value (bpath, gdk->outline_pixel, value);
		} else {
			get_color_value (bpath, 0, value);
		}
		break;

	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, priv->fill_rgba);
		break;

	case PROP_OUTLINE_COLOR_RGBA:
		g_value_set_uint (value, priv->fill_rgba);
		break;

	case PROP_FILL_STIPPLE:
		if (gdk) {
			g_value_set_boxed (value, gdk->fill_stipple);
//			GTK_VALUE_BOXED (*arg) = gdk->fill_stipple;
		} else {
			g_value_set_boxed (value, NULL);
//			GTK_VALUE_BOXED (*arg) = NULL;
		}
		break;

	case PROP_OUTLINE_STIPPLE:
		if (gdk) {
			g_value_set_boxed (value, gdk->outline_stipple);
//			GTK_VALUE_BOXED (*arg) = gdk->outline_stipple;
		} else {
			g_value_set_boxed (value, NULL);
//			GTK_VALUE_BOXED (*arg) = NULL;
		}
		break;

	case PROP_WIND:
		g_value_set_enum (value, priv->wind);
		break;

	case PROP_CAP_STYLE:
		g_value_set_enum (value, priv->cap);
		break;

	case PROP_JOIN_STYLE:
		g_value_set_enum (value, priv->join);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_bpath_realize (GnomeCanvasItem *item)
{
	GnomeCanvasBpath *bpath;

	bpath = GNOME_CANVAS_BPATH (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	if (!item->canvas->aa) {
		gcbp_ensure_gdk (bpath);

		bpath->priv->gdk->fill_gc = gdk_gc_new (item->canvas->layout.bin_window);
		bpath->priv->gdk->outline_gc = gdk_gc_new (item->canvas->layout.bin_window);
	}
}

static void
gnome_canvas_bpath_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasBpath *bpath;

	bpath = GNOME_CANVAS_BPATH (item);

	if (!item->canvas->aa) {
		g_assert (bpath->priv->gdk != NULL);

		gdk_gc_unref (bpath->priv->gdk->fill_gc);
		bpath->priv->gdk->fill_gc = NULL;

		gdk_gc_unref (bpath->priv->gdk->outline_gc);
		bpath->priv->gdk->outline_gc = NULL;
	}

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

static void
gnome_canvas_bpath_render (GnomeCanvasItem *item,
			     GnomeCanvasBuf *buf)
{
	GnomeCanvasBpath *bpath;

	bpath = GNOME_CANVAS_BPATH (item);

	if (bpath->priv->fill_svp != NULL)
		gnome_canvas_render_svp (buf,
			bpath->priv->fill_svp,
			bpath->priv->fill_rgba);

	if (bpath->priv->outline_svp != NULL)
		gnome_canvas_render_svp (buf,
			bpath->priv->outline_svp,
			bpath->priv->outline_rgba);
}

static void
gnome_canvas_bpath_draw (GnomeCanvasItem *item,
	GdkDrawable *drawable,
	int x,
	int y,
	int width,
	int height)
{
	static GdkPoint * dpoints = NULL;
	static gint num_dpoints = 0;

	GnomeCanvasBpath * bpath;
	GnomeCanvasBpathPriv * priv;
	GnomeCanvasBpathPrivGdk * gdk;
	gint i, pos, len;
	GSList * l;

	bpath = GNOME_CANVAS_BPATH (item);
	priv = bpath->priv;

	/* We have to be realized, so gdk struct should exist! */

	gdk = bpath->priv->gdk;
	g_assert (gdk != NULL);

	/* Build temporary point list, translated by -x, -y */

	if (dpoints == NULL) {
		dpoints = g_new (GdkPoint, gdk->num_points);
		num_dpoints = gdk->num_points;
	} else if (num_dpoints < gdk->num_points) {
		dpoints = g_renew (GdkPoint, dpoints, gdk->num_points);
		num_dpoints = gdk->num_points;
	}

	for (i = 0; i < gdk->num_points; i++) {
		dpoints[i].x = gdk->points[i].x - x;
		dpoints[i].y = gdk->points[i].y - y;
	}

	if (priv->fill_set) {

		/* Ensure, that we have mask and it is big enough */

		gcbp_ensure_mask (bpath, width, height);

		/* Clear mask */

		gdk_draw_rectangle (gdk->ctx->mask,
			gdk->ctx->clear_gc,
			TRUE,
			0, 0,
			width, height);

		/* Draw subpaths, using XOR gc */

		pos = 0;

		for (l = gdk->closed_paths; l != NULL; l = l->next) {
			len = GPOINTER_TO_INT (l->data);

			gdk_draw_polygon (gdk->ctx->mask,
				gdk->ctx->xor_gc,
				TRUE,
				&dpoints[pos],
				len);

			pos += len;
		}

		/* Set bitmap to clipping mask */

		gdk_gc_set_clip_mask (gdk->fill_gc, gdk->ctx->mask);

		/* Stipple offset */

		if (gdk->fill_stipple) gnome_canvas_set_stipple_origin (item->canvas, gdk->fill_gc);

		/* Draw clipped rect to drawable */

		gdk_draw_rectangle (drawable,
			gdk->fill_gc,
			TRUE,
			0, 0,
			width, height);
	}

	if (priv->outline_set) {

		/* Stipple offset */

		if (gdk->outline_stipple) gnome_canvas_set_stipple_origin (item->canvas, gdk->outline_gc);

		/* Draw subpaths */

		pos = 0;

		for (l = gdk->closed_paths; l != NULL; l = l->next) {
			len = GPOINTER_TO_INT (l->data);

			gdk_draw_polygon (drawable,
				gdk->outline_gc,
				FALSE,
				&dpoints[pos],
				len);

			pos += len;
		}

		for (l = gdk->open_paths; l != NULL; l = l->next) {
			len = GPOINTER_TO_INT (l->data);

			gdk_draw_polygon (drawable,
				gdk->outline_gc,
				FALSE,
				&dpoints[pos],
				len);

			pos += len;
		}
	}
}

static void
gnome_canvas_bpath_update_gdk (GnomeCanvasBpath * bpath, double * affine, ArtSVP * clip, int flags)
{
	GnomeCanvasBpathPriv * priv;
	GnomeCanvasBpathPrivGdk * gdk;

	g_assert (!((GnomeCanvasItem *) bpath)->canvas->aa);

	priv = bpath->priv;
	gdk = priv->gdk;
	g_assert (gdk != NULL);

	if (bpath->priv->outline_set) {
		GdkLineStyle style;
		gint width;

		if (priv->width_pixels) {
			width = (int) priv->width;
		} else {
			width = (int) (priv->width * priv->scale);
		}

		/* If dashed, set it in GdkGC */

		if ((bpath->priv->dash.dash != NULL) && (bpath->priv->dash.n_dash > 0)) {
			gint8 * dash_list;
			gint i;

			dash_list = g_new (gint8, bpath->priv->dash.n_dash);

			for (i = 0; i < priv->dash.n_dash; i++) {
				dash_list[i] = (gint8) bpath->priv->dash.dash[i];
			}

			gdk_gc_set_dashes (gdk->outline_gc,
				(gint) priv->dash.offset,
				dash_list,
				priv->dash.n_dash);

			g_free (dash_list);

			style = GDK_LINE_ON_OFF_DASH;
		} else {
			style = GDK_LINE_SOLID;
		}

		/* Set line width, cap, join */

		gdk_gc_set_line_attributes (gdk->outline_gc,
			width,
			style,
			priv->cap,
			priv->join);

		/* Colors and stipples */

		set_gc_foreground (gdk->outline_gc, gdk->outline_pixel);
		set_stipple (gdk->outline_gc, &gdk->outline_stipple, gdk->outline_stipple, TRUE);
	}

	if (priv->fill_set) {

		/* Colors and stipples */

		set_gc_foreground (gdk->fill_gc, gdk->fill_pixel);
		set_stipple (gdk->fill_gc, &gdk->fill_stipple, gdk->fill_stipple, TRUE);
	}

	/* Now the crazy part */

	/* Free existing GdkPoint array */

	if (gdk->points) {
		g_free (gdk->points);
		gdk->points = NULL;
		gdk->num_points = 0;
	}

	/* Free subpath lists */

	while (gdk->closed_paths) gdk->closed_paths = g_slist_remove (gdk->closed_paths, gdk->closed_paths->data);
	while (gdk->open_paths) gdk->open_paths = g_slist_remove (gdk->open_paths, gdk->open_paths->data);

	/* Calcualte new GdkPoints array and subpath lists */

	if (priv->path) {
		GPPath * apath, * cpath, * opath;
		ArtBpath * abpath;
		GSList * clist, * olist;
		gint pos;

		/* Allocate array */

		gdk->num_points = gp_path_length (priv->path) - 1;
		gdk->points = g_new (GdkPoint, gdk->num_points);

		/* Transform path */

		abpath = art_bpath_affine_transform (gp_path_bpath (priv->path), affine);
		apath = gp_path_new_from_bpath (abpath);

		/* Split path into open and closed parts */

		cpath = gp_path_closed_parts (apath);
		opath = gp_path_open_parts (apath);
		gp_path_unref (apath);

		/* Split partial paths into subpaths */

		clist = gp_path_split (cpath);
		gp_path_unref (cpath);
		olist = gp_path_split (opath);
		gp_path_unref (opath);

		pos = 0;

		/* Fill GdkPoints and add subpaths to list: closed subpaths */

		while (clist) {
			GPPath * path;
			ArtBpath * bpath;
			ArtVpath * vpath;
			gint len, i;

			path = (GPPath *) clist->data;
			bpath = gp_path_bpath (path);
			vpath = art_bez_path_to_vec (bpath, 0.5);
			for (len = 0; vpath[len].code != ART_END; len++) ;

			for (i = 0; i < len; i++) {
				gdk->points[pos + i].x = (gint16) vpath[i].x;
				gdk->points[pos + i].y = (gint16) vpath[i].y;
			}

			art_free (vpath);

			if (len > 0) {
				pos += len;
				gdk->closed_paths = g_slist_append (gdk->closed_paths, GINT_TO_POINTER (len));
			}

			gp_path_unref (path);
			clist = g_slist_remove (clist, clist->data);
		}

		/* Fill GdkPoints and add subpaths to list: open subpaths */

		while (olist) {
			GPPath * path;
			ArtBpath * bpath;
			ArtVpath * vpath;
			gint len, i;

			path = (GPPath *) olist->data;
			bpath = gp_path_bpath (path);
			vpath = art_bez_path_to_vec (bpath, 0.5);
			for (len = 0; vpath[len].code != ART_END; len++) ;

			for (i = 0; i < len; i++) {
				gdk->points[pos + i].x = (gint16) vpath[i].x;
				gdk->points[pos + i].y = (gint16) vpath[i].y;
			}

			art_free (vpath);

			if (len > 0) {
				pos += len;
				gdk->closed_paths = g_slist_append (gdk->closed_paths, GINT_TO_POINTER (len));
			}

			gp_path_unref (path);
			olist = g_slist_remove (olist, olist->data);
		}

	}

}

static void
gnome_canvas_bpath_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasBpath * bpath;
	GnomeCanvasBpathPriv * priv;
	ArtSVP * svp;

	bpath = GNOME_CANVAS_BPATH (item);

	priv = bpath->priv;

	/* Common part */

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	/* Outline width */

	bpath->priv->scale = (fabs (affine[0]) + fabs (affine[3])) / 2.0;

	/* Reset bbox */

	gnome_canvas_item_reset_bounds (item);
	
	/* Clipped fill SVP */

	if ((bpath->priv->fill_set) && (gp_path_any_closed (bpath->priv->path))) {
		GPPath * cpath;
		ArtBpath * abp;
		ArtVpath * vpath, * pvpath;
		ArtSVP *tmp_svp;

		/* Get closed part of path */

		cpath = gp_path_closed_parts (bpath->priv->path);
		abp = art_bpath_affine_transform (gp_path_bpath (cpath), affine);
		gp_path_unref (cpath);

		/* Render, until SVP */

		vpath = art_bez_path_to_vec (abp, 0.25);
		art_free (abp);

		pvpath = art_vpath_perturb (vpath);
		art_free (vpath);

		svp = art_svp_from_vpath (pvpath);
		art_free (pvpath);
		
		tmp_svp = art_svp_uncross (svp);
		art_svp_free (svp);

		svp = art_svp_rewind_uncrossed (tmp_svp, bpath->priv->wind);
		art_svp_free (tmp_svp);

		if (item->canvas->aa) {

			/* Update clipped path */

			gnome_canvas_item_update_svp_clip (item,
				&bpath->priv->fill_svp,
				svp,
				clip_path);
		} else {

			/* No clipping */

			gnome_canvas_item_update_svp_clip (item,
				&bpath->priv->fill_svp,
				svp,
				NULL);
		}
	}

	if ((priv->outline_set) && (!gp_path_is_empty (priv->path))) {
		gdouble width;
		ArtBpath * abp;
		ArtVpath * vpath, * pvpath;

		/* Set linewidth */

		if (priv->width_pixels) {
			width = priv->width;
		} else {
			width = priv->width * priv->scale;
		}
		
		if (width < 0.5) width = 0.5;
		
		/* Render full path until vpath */

		abp = art_bpath_affine_transform (gp_path_bpath (priv->path), affine);

		vpath = art_bez_path_to_vec (abp, 0.25);
		art_free (abp);

		pvpath = art_vpath_perturb (vpath);
		art_free (vpath);

		/* If dashed, apply dash */

		if (priv->dash.dash != NULL)
		{
			ArtVpath *old = pvpath;
			
			pvpath = art_vpath_dash (old, &priv->dash);
			art_free (old);
		}
		
		/* Stroke vpath to SVP */

		svp = art_svp_vpath_stroke (pvpath,
					    gnome_canvas_join_gdk_to_art (priv->join),
					    gnome_canvas_cap_gdk_to_art (priv->cap),
					    width,
					    priv->miterlimit,
					    0.25);
		art_free (pvpath);

		if (item->canvas->aa) {

			/* Update clipped */

			gnome_canvas_item_update_svp_clip (item, &priv->outline_svp, svp, clip_path);

		} else {

			/* No clipping (yet) */

			gnome_canvas_item_update_svp_clip (item, &priv->outline_svp, svp, NULL);
		}
	}

	/* Gdk requires additional handling */

	if (!item->canvas->aa) {
		gnome_canvas_bpath_update_gdk (bpath, affine, clip_path, flags);
	}
}

static double
gnome_canvas_bpath_point (GnomeCanvasItem *item, double x, double y,
			    int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasBpath *bpath;
	double dist;
	int wind;

	bpath = GNOME_CANVAS_BPATH (item);

	/* todo: update? */
	if (bpath->priv->fill_set) {
		wind = art_svp_point_wind (bpath->priv->fill_svp, cx, cy);
		if ((bpath->priv->wind == ART_WIND_RULE_NONZERO) && (wind != 0)) {
			*actual_item = item;
			return 0.0;
		}
		if ((bpath->priv->wind == ART_WIND_RULE_ODDEVEN) && ((wind & 0x1) != 0)) {
			*actual_item = item;
			return 0.0;
		}
	}

	if (bpath->priv->outline_set) {
		wind = art_svp_point_wind (bpath->priv->outline_svp, cx, cy);
		if (wind) {
			*actual_item = item;
			return 0.0;
		}
	}

	if (bpath->priv->outline_set) {
		dist = art_svp_point_dist (bpath->priv->outline_svp, cx, cy);
	} else if (bpath->priv->fill_set) {
		dist = art_svp_point_dist (bpath->priv->fill_svp, cx, cy);
	} else {
		return 1e12;
	}

	*actual_item = item;

	return dist;
}

/* Helpers */

/* Get 32bit rgba color from GdkColor */

static guint32
get_rgba_from_color (GdkColor * color)
{
	return ((color->red & 0xff00) << 16) | ((color->green & 0xff00) << 8) | (color->blue & 0xff00) | 0xff;
}

/* Get Gdk pixel value from 32bit rgba color */

static gulong
get_pixel_from_rgba (GnomeCanvasItem *item, guint32 rgba_color)
{
	return gnome_canvas_get_color_pixel (item->canvas, rgba_color);
}

/* Convenience function to set a GC's foreground color to the specified pixel value */

static void
set_gc_foreground (GdkGC *gc, gulong pixel)
{
	GdkColor c;

	g_assert (gc != NULL);

	c.pixel = pixel;

	gdk_gc_set_foreground (gc, &c);
}

/* Sets the stipple pattern for the specified gc */

static void
set_stipple (GdkGC *gc, GdkBitmap **internal_stipple, GdkBitmap *stipple, int reconfigure)
{
	if (*internal_stipple && !reconfigure)
		gdk_bitmap_unref (*internal_stipple);

	*internal_stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (gc) {
		if (stipple) {
			gdk_gc_set_stipple (gc, stipple);
			gdk_gc_set_fill (gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (gc, GDK_SOLID);
	}
}

/* Creates private Gdk struct, if not present */
/* We cannot do it during ::init, as we have to know canvas */

static void
gcbp_ensure_gdk (GnomeCanvasBpath * bpath)
{
	g_assert (!((GnomeCanvasItem *) bpath)->canvas->aa);

	if (!bpath->priv->gdk) {
		GnomeCanvasBpathPrivGdk * gdk;

		gdk = g_new (GnomeCanvasBpathPrivGdk, 1);

		gdk->fill_pixel = get_pixel_from_rgba ((GnomeCanvasItem *) bpath, bpath->priv->fill_rgba);
		gdk->outline_pixel = get_pixel_from_rgba ((GnomeCanvasItem *) bpath, bpath->priv->outline_rgba);

		gdk->fill_stipple = NULL;
		gdk->outline_stipple = NULL;

		gdk->fill_gc = NULL;
		gdk->outline_gc = NULL;

		gdk->num_points = 0;
		gdk->points = NULL;
		gdk->closed_paths = NULL;
		gdk->open_paths = NULL;

		gdk->ctx = NULL;

		bpath->priv->gdk = gdk;
	}
}

/* Destroy private Gdk struct */
/* It is here, to make ::destroy implementation shorter :) */

static void
gcbp_destroy_gdk (GnomeCanvasBpath * bpath)
{
	GnomeCanvasBpathPrivGdk * gdk;

	g_assert (!((GnomeCanvasItem *)bpath)->canvas->aa);

	gdk = bpath->priv->gdk;

	if (gdk) {
		g_assert (!gdk->fill_gc);
		g_assert (!gdk->outline_gc);

		if (gdk->fill_stipple)
			gdk_bitmap_unref (gdk->fill_stipple);

		if (gdk->outline_stipple)
			gdk_bitmap_unref (gdk->outline_stipple);

		if (gdk->points)
			g_free (gdk->points);

		while (gdk->closed_paths)
			gdk->closed_paths = g_slist_remove (gdk->closed_paths, gdk->closed_paths->data);
		while (gdk->open_paths)
			gdk->open_paths = g_slist_remove (gdk->open_paths, gdk->open_paths->data);

		if (gdk->ctx)
			gcbp_draw_ctx_unref (gdk->ctx);

		g_free (gdk);

		bpath->priv->gdk = NULL;
	}
}

/*
 * Ensure, that per-canvas Ctx struct is present and bitmaps are
 * big enough, to mask full redraw area. Ctx is refcounted and
 * defined as "BpathDrawCtx" data member on parent canvas
 */

static void
gcbp_ensure_mask (GnomeCanvasBpath * bpath, gint width, gint height)
{
	GnomeCanvasBpathPrivGdk * gdk;
	GCBPDrawCtx * ctx;

	gdk = bpath->priv->gdk;
	g_assert (gdk != NULL);
	ctx = gdk->ctx;

	if (!ctx) {
		/* Ctx is not yet defined for us */

		GnomeCanvas * canvas;

		canvas = GNOME_CANVAS_ITEM (bpath)->canvas;

		ctx = gtk_object_get_data (GTK_OBJECT (canvas), "BpathDrawCtx");

		if (!ctx) {
			/* Ctx is not defined for parent canvas yet */

			GdkWindow * window;

			window = ((GtkWidget *) (((GnomeCanvasItem *) bpath)->canvas))->window;

			ctx = g_new (GCBPDrawCtx, 1);

			ctx->refcount = 1;
			ctx->canvas = canvas;
			ctx->width = 0;
			ctx->height = 0;

			ctx->mask = NULL;
			ctx->clip = NULL;

			ctx->clear_gc = NULL;
			ctx->xor_gc = NULL;

			gtk_object_set_data (GTK_OBJECT (canvas), "BpathDrawContext", ctx);

		} else {
			ctx->refcount++;
		}

		gdk->ctx = ctx;

	}

	/* Now we are sure, that ctx is present and properly refcounted */

	if ((width > ctx->width) || (height > ctx->height)) {
		/* Ctx is too small */

		GdkWindow * window;

		window = ((GtkWidget *) (((GnomeCanvasItem *) bpath)->canvas))->window;

		if (ctx->clear_gc) gdk_gc_unref (ctx->clear_gc);
		if (ctx->xor_gc) gdk_gc_unref (ctx->xor_gc);
		if (ctx->mask) gdk_bitmap_unref (ctx->mask);
		if (ctx->clip) gdk_bitmap_unref (ctx->clip);

		ctx->mask = gdk_pixmap_new (window, width, height, 1);
		ctx->clip = NULL;

		ctx->clear_gc = gdk_gc_new (ctx->mask);
		gdk_gc_set_function (ctx->clear_gc, GDK_CLEAR);

		ctx->xor_gc = gdk_gc_new (ctx->mask);
		gdk_gc_set_function (ctx->xor_gc, GDK_INVERT);
	}
}

/* It is cleaner to have it here, not in parent function */

static void
gcbp_draw_ctx_unref (GCBPDrawCtx * ctx)
{
	if (--ctx->refcount < 1) {
		if (ctx->clear_gc)
			gdk_gc_unref (ctx->clear_gc);
		if (ctx->xor_gc)
			gdk_gc_unref (ctx->xor_gc);

		if (ctx->mask)
			gdk_bitmap_unref (ctx->mask);
		if (ctx->clip)
			gdk_bitmap_unref (ctx->clip);

		gtk_object_remove_data (GTK_OBJECT (ctx->canvas), "BpathDrawCtx");
	}
}

