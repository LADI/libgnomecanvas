#include <config.h>
#include "testgnome.h"


#ifdef GTK_HAVE_ACCEL_GROUP


/*** Primitives ***/

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

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		x = event->button.x;
		y = event->button.y;
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

static GtkWidget *
create_primitives (void)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GtkWidget *frame;
	GtkWidget *canvas;
	GtkAdjustment *adj;
	GnomeCanvasGroup *root;
	GdkImlibImage *im;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

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

	gnome_canvas_set_size (GNOME_CANVAS (canvas), 300, 300);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	root = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (canvas)));

	setup_item (gnome_canvas_item_new (GNOME_CANVAS (canvas),
					   root,
					   gnome_canvas_rect_get_type (),
					   "GnomeCanvasRE::x1", 10.0,
					   "GnomeCanvasRE::y1", 10.0,
					   "GnomeCanvasRE::x2", 160.0,
					   "GnomeCanvasRE::y2", 60.0,
					   "GnomeCanvasRE::fill_color", "mediumseagreen",
					   "GnomeCanvasRE::outline_color", "black",
					   "GnomeCanvasRE::width_pixels", 4,
					   NULL));

	setup_item (gnome_canvas_item_new (GNOME_CANVAS (canvas),
					   root,
					   gnome_canvas_ellipse_get_type (),
					   "GnomeCanvasRE::x1", 20.0,
					   "GnomeCanvasRE::y1", 70.0,
					   "GnomeCanvasRE::x2", 100.0,
					   "GnomeCanvasRE::y2", 130.0,
					   "GnomeCanvasRE::fill_color", "tan",
					   "GnomeCanvasRE::outline_color", "slateblue",
					   "GnomeCanvasRE::width_units", 6.0,
					   NULL));

	setup_item (gnome_canvas_item_new (GNOME_CANVAS (canvas),
					   root,
					   gnome_canvas_text_get_type (),
					   "GnomeCanvasText::text", "Hello, world!",
					   "GnomeCanvasText::x", 200.0,
					   "GnomeCanvasText::y", 100.0,
					   "GnomeCanvasText::font", "-adobe-helvetica-bold-r-normal--24-240-75-75-p-138-iso8859-1",
					   "GnomeCanvasText::anchor", GTK_ANCHOR_CENTER,
					   "GnomeCanvasText::fill_color", "blue",
					   NULL));

	im = gdk_imlib_load_image ("toroid.png");
	setup_item (gnome_canvas_item_new (GNOME_CANVAS (canvas),
					   root,
					   gnome_canvas_image_get_type (),
					   "GnomeCanvasImage::image", im,
					   "GnomeCanvasImage::x", 100.0,
					   "GnomeCanvasImage::y", 200.0,
					   "GnomeCanvasImage::width", (double) im->rgb_width,
					   "GnomeCanvasImage::height", (double) im->rgb_height,
					   "GnomeCanvasImage::anchor", GTK_ANCHOR_CENTER,
					   NULL));

	return vbox;
}


/*** Fifteen ***/


#define PIECE_SIZE 50


static void
free_stuff (GtkObject *obj, gpointer data)
{
	g_free (data);
}

static void
test_win (GnomeCanvasItem **board)
{
	int i;

	for (i = 0; i < 15; i++)
		if (!board[i] || (GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (board[i]), "piece_num")) != i))
			return;

	gnome_dialog_run_modal (GNOME_DIALOG (gnome_ok_dialog ("You stud, you win!")));
}

static char *
get_piece_color (int piece)
{
	static char buf[50];
	int x, y;
	int r, g, b;

	y = piece / 4;
	x = piece % 4;

	r = ((4 - x) * 255) / 4;
	g = ((4 - y) * 255) / 4;
	b = 128;

	sprintf (buf, "#%02x%02x%02x", r, g, b);

	return buf;
}

static gint
piece_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	GnomeCanvas *canvas;
	GnomeCanvasItem **board;
	GnomeCanvasItem *text;
	int num, pos, newpos;
	int x, y;
	double dx, dy;
	int move;

	canvas = item->canvas;
	board = gtk_object_get_user_data (GTK_OBJECT (canvas));
	num = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "piece_num"));
	pos = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "piece_pos"));
	text = gtk_object_get_data (GTK_OBJECT (item), "text");

	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		gnome_canvas_item_set (text,
				       "GnomeCanvasText::fill_color", "white",
				       NULL);
		break;

	case GDK_LEAVE_NOTIFY:
		gnome_canvas_item_set (text,
				       "GnomeCanvasText::fill_color", "black",
				       NULL);
		break;

	case GDK_BUTTON_PRESS:
		y = pos / 4;
		x = pos % 4;

		move = TRUE;

		if ((y > 0) && (board[(y - 1) * 4 + x] == NULL)) {
			dx = 0.0;
			dy = -1.0;
			y--;
		} else if ((y < 3) && (board[(y + 1) * 4 + x] == NULL)) {
			dx = 0.0;
			dy = 1.0;
			y++;
		} else if ((x > 0) && (board[y * 4 + x - 1] == NULL)) {
			dx = -1.0;
			dy = 0.0;
			x--;
		} else if ((x < 3) && (board[y * 4 + x + 1] == NULL)) {
			dx = 1.0;
			dy = 0.0;
			x++;
		} else
			move = FALSE;

		if (move) {
			newpos = y * 4 + x;
			board[pos] = NULL;
			board[newpos] = item;
			gtk_object_set_data (GTK_OBJECT (item), "piece_pos", GINT_TO_POINTER (newpos));
			gnome_canvas_item_move (item, dx * PIECE_SIZE, dy * PIECE_SIZE);
			test_win (board);
		}

		break;

	default:
		break;
	}

	return FALSE;
}

#define SCRAMBLE_MOVES 256

static void
scramble (GtkObject *object, gpointer data)
{
	GnomeCanvas *canvas;
	GnomeCanvasItem **board;
	int i;
	int pos, oldpos;
	int dir;
	int x, y;

	srand (time (NULL));

	canvas = data;
	board = gtk_object_get_user_data (object);

	/* First, find the blank spot */

	for (pos = 0; pos < 16; pos++)
		if (board[pos] == NULL)
			break;

	/* "Move the blank spot" around in order to scramble the pieces */

	for (i = 0; i < SCRAMBLE_MOVES; i++) {
retry_scramble:
		dir = rand () % 4;

		x = y = 0;

		if ((dir == 0) && (pos > 3)) /* up */
			y = -1;
		else if ((dir == 1) && (pos < 12)) /* down */
			y = 1;
		else if ((dir == 2) && ((pos % 4) != 0)) /* left */
			x = -1;
		else if ((dir == 3) && ((pos % 4) != 3)) /* right */
			x = 1;
		else
			goto retry_scramble;

		oldpos = pos + y * 4 + x;
		board[pos] = board[oldpos];
		board[oldpos] = NULL;
		gtk_object_set_data (GTK_OBJECT (board[pos]), "piece_pos", GINT_TO_POINTER (pos));
		gnome_canvas_item_move (board[pos], -x * PIECE_SIZE, -y * PIECE_SIZE);
		gnome_canvas_update_now (canvas);
		pos = oldpos;
	}
}

static GtkWidget *
create_fifteen (void)
{
	GtkWidget *vbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *canvas;
	GtkWidget *button;
	GnomeCanvasItem **board;
	GnomeCanvasItem *text;
	int i, x, y;
	char buf[20];

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, TRUE, TRUE, 0);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	/* Create the canvas and board */

	canvas = gnome_canvas_new (gtk_widget_get_default_visual (), gtk_widget_get_default_colormap ());
	gnome_canvas_set_size (GNOME_CANVAS (canvas), PIECE_SIZE * 4, PIECE_SIZE * 4);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	board = g_new (GnomeCanvasItem *, 16);
	gtk_object_set_user_data (GTK_OBJECT (canvas), board);
	gtk_signal_connect (GTK_OBJECT (canvas), "destroy",
			    (GtkSignalFunc) free_stuff,
			    board);

	for (i = 0; i < 15; i++) {
		y = i / 4;
		x = i % 4;

		board[i] = gnome_canvas_item_new (GNOME_CANVAS (canvas),
						  GNOME_CANVAS_GROUP (GNOME_CANVAS (canvas)->root),
						  gnome_canvas_group_get_type (),
						  "GnomeCanvasGroup::x", (double) (x * PIECE_SIZE),
						  "GnomeCanvasGroup::y", (double) (y * PIECE_SIZE),
						  NULL);

		gnome_canvas_item_new (GNOME_CANVAS (canvas),
				       GNOME_CANVAS_GROUP (board[i]),
				       gnome_canvas_rect_get_type (),
				       "GnomeCanvasRE::x1", 0.0,
				       "GnomeCanvasRE::y1", 0.0,
				       "GnomeCanvasRE::x2", (double) (PIECE_SIZE - 1),
				       "GnomeCanvasRE::y2", (double) (PIECE_SIZE - 1),
				       "GnomeCanvasRE::fill_color", get_piece_color (i),
				       "GnomeCanvasRE::outline_color", "black",
				       "GnomeCanvasRE::width_pixels", 0,
				       NULL);

		sprintf (buf, "%d", i + 1);

		text = gnome_canvas_item_new (GNOME_CANVAS (canvas),
					      GNOME_CANVAS_GROUP (board[i]),
					      gnome_canvas_text_get_type (),
					      "GnomeCanvasText::text", buf,
					      "GnomeCanvasText::x", (double) PIECE_SIZE / 2.0,
					      "GnomeCanvasText::y", (double) PIECE_SIZE / 2.0,
					      "GnomeCanvasText::font", "-adobe-helvetica-bold-r-normal--24-240-75-75-p-138-iso8859-1",
					      "GnomeCanvasText::anchor", GTK_ANCHOR_CENTER,
					      "GnomeCanvasText::fill_color", "black",
					      NULL);

		gtk_object_set_data (GTK_OBJECT (board[i]), "piece_num", GINT_TO_POINTER (i));
		gtk_object_set_data (GTK_OBJECT (board[i]), "piece_pos", GINT_TO_POINTER (i));
		gtk_object_set_data (GTK_OBJECT (board[i]), "text", text);
		gtk_signal_connect (GTK_OBJECT (board[i]), "event",
				    (GtkSignalFunc) piece_event,
				    NULL);
	}

	board[15] = NULL;

	/* Scramble button */

	button = gtk_button_new_with_label ("Scramble");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_object_set_user_data (GTK_OBJECT (button), board);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) scramble,
			    canvas);
	gtk_widget_show (button);

	return vbox;
}


/*** Main canvas test ***/


void
create_canvas (void)
{
	GtkWidget *app;
	GtkWidget *notebook;

	app = create_newwin (TRUE, "testGNOME", "Canvas");

	notebook = gtk_notebook_new ();
	gnome_app_set_contents (GNOME_APP (app), notebook);
	gtk_widget_show (notebook);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_primitives (), gtk_label_new ("Primitives"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_fifteen (), gtk_label_new ("Fifteen"));

	gtk_widget_show (app);
}

#endif
