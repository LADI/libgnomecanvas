#include <config.h>
#include <math.h>
#include "testgnome.h"

#ifdef GTK_HAVE_FEATURES_1_1_0


static void
zoom_changed (GtkAdjustment *adj, gpointer data)
{
	gnome_canvas_set_pixels_per_unit (data, adj->value);
}

static gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	static double x, y;
	double new_x, new_y;
	GdkCursor *fleur;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			if (event->button.state & GDK_SHIFT_MASK)
				gtk_object_destroy (GTK_OBJECT (item));
			else {
				x = event->button.x;
				y = event->button.y;

				fleur = gdk_cursor_new (GDK_FLEUR);
				gnome_canvas_item_grab (item,
							GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
							fleur,
							event->button.time);
				gdk_cursor_destroy (fleur);
			}
			break;

		case 2:
			if (event->button.state & GDK_SHIFT_MASK)
				gnome_canvas_item_lower_to_bottom (item);
			else
				gnome_canvas_item_lower (item, 1);
			break;

		case 3:
			if (event->button.state & GDK_SHIFT_MASK)
				gnome_canvas_item_raise_to_top (item);
			else
				gnome_canvas_item_raise (item, 1);
			break;

		default:
			break;
		}

		break;

	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			new_x = event->motion.x;
			new_y = event->motion.y;

			gnome_canvas_item_move (item, new_x - x, new_y - y);
			x = new_x;
			y = new_y;
		}
		break;

	case GDK_BUTTON_RELEASE:
		gnome_canvas_item_ungrab (item, event->button.time);

	default:
		break;
	}

	return FALSE;
}

static void
setup_item (GnomeCanvasItem *item)
{
	gtk_signal_connect (GTK_OBJECT (item), "event",
			    (GtkSignalFunc) item_event,
			    NULL);
}

#define SCROLL_DIST 5

static void
scroll (GtkWidget *widget, gpointer data)
{
	GtkArrowType type;
	GnomeCanvas *canvas;
	int dx, dy;

	type = GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (widget)));
	canvas = data;

	dx = 0;
	dy = 0;

	switch (type) {
	case GTK_ARROW_LEFT:
		dx = -SCROLL_DIST;
		break;

	case GTK_ARROW_RIGHT:
		dx = SCROLL_DIST;
		break;

	case GTK_ARROW_UP:
		dy = -SCROLL_DIST;
		break;

	case GTK_ARROW_DOWN:
		dy = SCROLL_DIST;
		break;
	}

	gnome_canvas_scroll_to (canvas, canvas->display_x1 + dx, canvas->display_y1 + dy);
}

static GtkWidget *
make_arrow (GtkArrowType type, gpointer data)
{
	GtkWidget *w;

	w = gtk_button_new ();
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    (GtkSignalFunc) scroll,
			    data);
	gtk_container_add (GTK_CONTAINER (w), gtk_arrow_new (type, GTK_SHADOW_IN));
	gtk_object_set_user_data (GTK_OBJECT (w), GINT_TO_POINTER (type));
	gtk_widget_show_all (w);

	return w;
}

static void
setup_heading (GnomeCanvasGroup *root, char *text, int pos)
{
	gnome_canvas_item_new (root,
			       gnome_canvas_text_get_type (),
			       "text", text,
			       "x", (double) ((pos % 3) * 200 + 100),
			       "y", (double) ((pos / 3) * 150 + 5),
			       "font", "-adobe-helvetica-medium-r-normal--12-*-72-72-p-*-iso8859-1",
			       "anchor", GTK_ANCHOR_N,
			       "fill_color", "black",
			       NULL);
}

static void
setup_divisions (GnomeCanvasGroup *root)
{
	GnomeCanvasGroup *group;
	GnomeCanvasPoints *points;

	group = GNOME_CANVAS_GROUP (gnome_canvas_item_new (root,
							   gnome_canvas_group_get_type (),
							   "x", 0.0,
							   "y", 0.0,
							   NULL));
	setup_item (GNOME_CANVAS_ITEM (group));

	points = gnome_canvas_points_new (2);

	gnome_canvas_item_new (group,
			       gnome_canvas_rect_get_type (),
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", 600.0,
			       "y2", 450.0,
			       "outline_color", "black",
			       "width_units", 4.0,
			       NULL);

	points->coords[0] = 0.0;
	points->coords[1] = 150.0;
	points->coords[2] = 600.0;
	points->coords[3] = 150.0;
	gnome_canvas_item_new (group,
			       gnome_canvas_line_get_type (),
			       "points", points,
			       "fill_color", "black",
			       "width_units", 4.0,
			       NULL);

	points->coords[0] = 0.0;
	points->coords[1] = 300.0;
	points->coords[2] = 600.0;
	points->coords[3] = 300.0;
	gnome_canvas_item_new (group,
			       gnome_canvas_line_get_type (),
			       "points", points,
			       "fill_color", "black",
			       "width_units", 4.0,
			       NULL);

	points->coords[0] = 200.0;
	points->coords[1] = 0.0;
	points->coords[2] = 200.0;
	points->coords[3] = 450.0;
	gnome_canvas_item_new (group,
			       gnome_canvas_line_get_type (),
			       "points", points,
			       "fill_color", "black",
			       "width_units", 4.0,
			       NULL);

	points->coords[0] = 400.0;
	points->coords[1] = 0.0;
	points->coords[2] = 400.0;
	points->coords[3] = 450.0;
	gnome_canvas_item_new (group,
			       gnome_canvas_line_get_type (),
			       "points", points,
			       "fill_color", "black",
			       "width_units", 4.0,
			       NULL);

	setup_heading (group, "Rectangles", 0);
	setup_heading (group, "Ellipses", 1);
	setup_heading (group, "Texts", 2);
	setup_heading (group, "Images", 3);
	setup_heading (group, "Lines", 4);
	setup_heading (group, "Curves", 5);
}

static void
setup_rectangles (GnomeCanvasGroup *root)
{
	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_rect_get_type (),
					   "x1", 20.0,
					   "y1", 30.0,
					   "x2", 70.0,
					   "y2", 60.0,
					   "outline_color", "red",
					   "width_pixels", 8,
					   NULL));

	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_rect_get_type (),
					   "x1", 90.0,
					   "y1", 40.0,
					   "x2", 180.0,
					   "y2", 100.0,
					   "fill_color", "mediumseagreen",
					   "outline_color", "black",
					   "width_units", 4.0,
					   NULL));

	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_rect_get_type (),
					   "x1", 10.0,
					   "y1", 80.0,
					   "x2", 80.0,
					   "y2", 140.0,
					   "fill_color", "steelblue",
					   NULL));
}

static void
setup_ellipses (GnomeCanvasGroup *root)
{
	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_ellipse_get_type (),
					   "x1", 220.0,
					   "y1", 30.0,
					   "x2", 270.0,
					   "y2", 60.0,
					   "outline_color", "goldenrod",
					   "width_pixels", 8,
					   NULL));

	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_ellipse_get_type (),
					   "x1", 290.0,
					   "y1", 40.0,
					   "x2", 380.0,
					   "y2", 100.0,
					   "fill_color", "wheat",
					   "outline_color", "midnightblue",
					   "width_units", 4.0,
					   NULL));

	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_ellipse_get_type (),
					   "x1", 210.0,
					   "y1", 80.0,
					   "x2", 280.0,
					   "y2", 140.0,
					   "fill_color", "cadetblue",
					   NULL));
}

static GnomeCanvasGroup *
make_anchor (GnomeCanvasGroup *root, double x, double y)
{
	GnomeCanvasGroup *group;

	group = GNOME_CANVAS_GROUP (gnome_canvas_item_new (root,
							   gnome_canvas_group_get_type (),
							   "x", x,
							   "y", y,
							   NULL));
	setup_item (GNOME_CANVAS_ITEM (group));

	gnome_canvas_item_new (group,
			       gnome_canvas_rect_get_type (),
			       "x1", -2.0,
			       "y1", -2.0,
			       "x2", 2.0,
			       "y2", 2.0,
			       "outline_color", "black",
			       "width_pixels", 0,
			       NULL);

	return group;
}

static void
setup_texts (GnomeCanvasGroup *root)
{
	gnome_canvas_item_new (make_anchor (root, 420.0, 40.0),
			       gnome_canvas_text_get_type (),
			       "text", "Anchor west",
			       "x", 0.0,
			       "y", 0.0,
			       "font", "-adobe-helvetica-bold-r-normal--24-240-75-75-p-138-iso8859-1",
			       "anchor", GTK_ANCHOR_W,
			       "fill_color", "blue",
			       NULL);

	gnome_canvas_item_new (make_anchor (root, 500.0, 75.0),
			       gnome_canvas_text_get_type (),
			       "text", "Anchor center",
			       "x", 0.0,
			       "y", 0.0,
			       "font", "-bitstream-charter-bold-*-normal--20-*-0-0-p-*-iso8859-1",
			       "anchor", GTK_ANCHOR_CENTER,
			       "fill_color", "firebrick",
			       NULL);

	gnome_canvas_item_new (make_anchor (root, 500.0, 130.0),
			       gnome_canvas_text_get_type (),
			       "text", "Anchor south",
			       "x", 0.0,
			       "y", 0.0,
			       "font", "-bitstream-courier-bold-r-normal--25-*-0-0-m-*-iso8859-1",
			       "anchor", GTK_ANCHOR_S,
			       "fill_color", "darkgreen",
			       NULL);
}

static void
free_imlib_image (GtkObject *object, gpointer data)
{
	gdk_imlib_destroy_image (data);
}

static void
plant_flower (GnomeCanvasGroup *root, double x, double y, GtkAnchorType anchor)
{
	GdkImlibImage *im;
	GnomeCanvasItem *image;

	im = gdk_imlib_load_image ("flower.png");
	image = gnome_canvas_item_new (root,
				       gnome_canvas_image_get_type (),
				       "image", im,
				       "x", x,
				       "y", y,
				       "width", (double) im->rgb_width,
				       "height", (double) im->rgb_height,
				       "anchor", anchor,
				       NULL);
	setup_item (image);
	gtk_signal_connect (GTK_OBJECT (image), "destroy",
			    (GtkSignalFunc) free_imlib_image,
			    im);
}

static void
setup_images (GnomeCanvasGroup *root)
{
	GdkImlibImage *im;
	GnomeCanvasItem *image;

	im = gdk_imlib_load_image ("toroid.png");
	image = gnome_canvas_item_new (root,
				       gnome_canvas_image_get_type (),
				       "image", im,
				       "x", 100.0,
				       "y", 225.0,
				       "width", (double) im->rgb_width,
				       "height", (double) im->rgb_height,
				       "anchor", GTK_ANCHOR_CENTER,
				       NULL);
	setup_item (image);
	gtk_signal_connect (GTK_OBJECT (image), "destroy",
			    (GtkSignalFunc) free_imlib_image,
			    im);

	plant_flower (root,  20.0, 170.0, GTK_ANCHOR_NW);
	plant_flower (root, 180.0, 170.0, GTK_ANCHOR_NE);
	plant_flower (root,  20.0, 280.0, GTK_ANCHOR_SW);
	plant_flower (root, 180.0, 280.0, GTK_ANCHOR_SE);
}

#define VERTICES 10
#define RADIUS 60.0

static void
polish_diamond (GnomeCanvasGroup *root)
{
	GnomeCanvasGroup *group;
	int i, j;
	double a;
	GnomeCanvasPoints *points;

	group = GNOME_CANVAS_GROUP (gnome_canvas_item_new (root,
							   gnome_canvas_group_get_type (),
							   "x", 270.0,
							   "y", 230.0,
							   NULL));
	setup_item (GNOME_CANVAS_ITEM (group));

	points = gnome_canvas_points_new (2);

	for (i = 0; i < VERTICES; i++) {
		a = 2.0 * M_PI * i / VERTICES;
		points->coords[0] = RADIUS * cos (a);
		points->coords[1] = RADIUS * sin (a);

		for (j = i + 1; j < VERTICES; j++) {
			a = 2.0 * M_PI * j / VERTICES;
			points->coords[2] = RADIUS * cos (a);
			points->coords[3] = RADIUS * sin (a);
			gnome_canvas_item_new (group,
					       gnome_canvas_line_get_type (),
					       "points", points,
					       "fill_color", "black",
					       "width_units", 1.0,
					       "cap_style", GDK_CAP_ROUND,
					       NULL);
		}
	}

	gnome_canvas_points_free (points);
}

#define SCALE 7.0

static void
make_hilbert (GnomeCanvasGroup *root)
{
	char hilbert[] = "urdrrulurulldluuruluurdrurddldrrruluurdrurddldrddlulldrdldrrurd";
	char *c;
	double *pp, *p;
	GnomeCanvasPoints *points;

	points = gnome_canvas_points_new (strlen (hilbert) + 1);
	points->coords[0] = 340.0;
	points->coords[1] = 290.0;

	pp = points->coords;
	for (c = hilbert, p = points->coords + 2; *c; c++, p += 2, pp += 2)
		switch (*c) {
		case 'u':
			p[0] = pp[0];
			p[1] = pp[1] - SCALE;
			break;

		case 'd':
			p[0] = pp[0];
			p[1] = pp[1] + SCALE;
			break;

		case 'l':
			p[0] = pp[0] - SCALE;
			p[1] = pp[1];
			break;

		case 'r':
			p[0] = pp[0] + SCALE;
			p[1] = pp[1];
			break;
		}

	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_line_get_type (),
					   "points", points,
					   "fill_color", "red",
					   "width_units", 3.0,
					   "cap_style", GDK_CAP_PROJECTING,
					   "join_style", GDK_JOIN_MITER,
					   NULL));

	gnome_canvas_points_free (points);
}

static void
setup_lines (GnomeCanvasGroup *root)
{
	GnomeCanvasPoints *points;

	polish_diamond (root);
	make_hilbert (root);

	/* Arrow tests */

	points = gnome_canvas_points_new (4);
	points->coords[0] = 340.0;
	points->coords[1] = 170.0;
	points->coords[2] = 340.0;
	points->coords[3] = 230.0;
	points->coords[4] = 390.0;
	points->coords[5] = 230.0;
	points->coords[6] = 390.0;
	points->coords[7] = 170.0;
	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_line_get_type (),
					   "points", points,
					   "fill_color", "midnightblue",
					   "width_units", 3.0,
					   "first_arrowhead", TRUE,
					   "last_arrowhead", TRUE,
					   "arrow_shape_a", 8.0,
					   "arrow_shape_b", 12.0,
					   "arrow_shape_c", 4.0,
					   NULL));
	gnome_canvas_points_free (points);

	points = gnome_canvas_points_new (2);
	points->coords[0] = 356.0;
	points->coords[1] = 180.0;
	points->coords[2] = 374.0;
	points->coords[3] = 220.0;
	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_line_get_type (),
					   "points", points,
					   "fill_color", "blue",
					   "width_pixels", 0,
					   "first_arrowhead", TRUE,
					   "last_arrowhead", TRUE,
					   "arrow_shape_a", 6.0,
					   "arrow_shape_b", 6.0,
					   "arrow_shape_c", 4.0,
					   NULL));

	points->coords[0] = 356.0;
	points->coords[1] = 220.0;
	points->coords[2] = 374.0;
	points->coords[3] = 180.0;
	setup_item (gnome_canvas_item_new (root,
					   gnome_canvas_line_get_type (),
					   "points", points,
					   "fill_color", "blue",
					   "width_pixels", 0,
					   "first_arrowhead", TRUE,
					   "last_arrowhead", TRUE,
					   "arrow_shape_a", 6.0,
					   "arrow_shape_b", 6.0,
					   "arrow_shape_c", 4.0,
					   NULL));
	gnome_canvas_points_free (points);
}

GtkWidget *
create_canvas_primitives (void)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GtkWidget *frame;
	GtkWidget *canvas;
	GtkAdjustment *adj;
	GnomeCanvasGroup *root;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	w = gtk_label_new ("Drag an item with button 1.  Click button 2 on an item to lower it,\n"
			   "or button 3 to raise it.  Shift+click with buttons 2 or 3 to send\n"
			   "an item to the bottom or top, respectively.");
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	canvas = gnome_canvas_new (gdk_imlib_get_visual (), gdk_imlib_get_colormap ());

	/* Zoom */

	w = gtk_label_new ("Zoom:");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.00, 0.05, 5.00, 0.05, 0.50, 0.50));
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    (GtkSignalFunc) zoom_changed,
			    canvas);
	w = gtk_spin_button_new (adj, 0.0, 2);
	gtk_widget_set_usize (w, 50, 0);
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	/* Scroll buttons */

	gtk_box_pack_start (GTK_BOX (hbox), make_arrow (GTK_ARROW_LEFT, canvas), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), make_arrow (GTK_ARROW_DOWN, canvas), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), make_arrow (GTK_ARROW_UP, canvas), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), make_arrow (GTK_ARROW_RIGHT, canvas), FALSE, FALSE, 0);

	/* Canvas */

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	gnome_canvas_set_size (GNOME_CANVAS (canvas), 600, 450);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	root = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (canvas)));

	setup_divisions (root);
	setup_rectangles (root);
	setup_ellipses (root);
	setup_texts (root);
	setup_images (root);
	setup_lines (root);

	return vbox;
}


#endif
