#include <config.h>
#include "testgnome.h"


#ifdef GTK_HAVE_ACCEL_GROUP


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
	int num, pos, newpos;
	int x, y;
	double dx, dy;

	canvas = item->canvas;
	board = gtk_object_get_user_data (GTK_OBJECT (canvas));
	num = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "piece_num"));
	pos = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "piece_pos"));

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		y = pos / 4;
		x = pos % 4;

		/* up */

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
		}

		newpos = y * 4 + x;
		board[pos] = NULL;
		board[newpos] = item;
		gtk_object_set_data (GTK_OBJECT (item), "piece_pos", GINT_TO_POINTER (newpos));
		gnome_canvas_item_move (item, dx * PIECE_SIZE, dy * PIECE_SIZE);
		test_win (board);
		break;

	default:
		break;
	}

	return FALSE;
}

static GtkWidget *
create_fifteen (void)
{
	GtkWidget *frame;
	GtkWidget *canvas;
	GnomeCanvasItem **board;
	int i, x, y;

	frame = gtk_frame_new (NULL);
	gtk_container_border_width (GTK_CONTAINER (frame), 4);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_widget_show (frame);

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
						  gnome_canvas_rect_get_type (),
						  "GnomeCanvasRE::x1", (double) (x * PIECE_SIZE),
						  "GnomeCanvasRE::y1", (double) (y * PIECE_SIZE),
						  "GnomeCanvasRE::x2", (double) ((x + 1) * PIECE_SIZE - 1),
						  "GnomeCanvasRE::y2", (double) ((y + 1) * PIECE_SIZE - 1),
						  "GnomeCanvasRE::fill_color", get_piece_color (i),
						  "GnomeCanvasRE::outline_color", "black",
						  "GnomeCanvasRE::width_pixels", 0,
						  NULL);
		gtk_object_set_data (GTK_OBJECT (board[i]), "piece_num", GINT_TO_POINTER (i));
		gtk_object_set_data (GTK_OBJECT (board[i]), "piece_pos", GINT_TO_POINTER (i));
		gtk_signal_connect (GTK_OBJECT (board[i]), "event",
				    (GtkSignalFunc) piece_event,
				    NULL);
	}

	board[15] = NULL;

	return frame;
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

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_fifteen (), gtk_label_new ("Fifteen"));

	gtk_widget_show (app);
}

#endif
