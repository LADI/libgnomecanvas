#include "testgnome.h"


static void
set_pixels_per_unit (GtkAdjustment *adj, GnomeCanvas *canvas)
{
	gnome_canvas_set_pixels_per_unit (canvas, adj->value);
}

static gint
item_event (GnomeCanvas *canvas, GnomeCanvasId *item, GdkEvent *event, gpointer item_data, gpointer user_data)
{
	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		gnome_canvas_configure (canvas, *item,
					GNOME_CANVAS_FILL_COLOR, GNOME_CANVAS_COLOR_STRING, "white",
					GNOME_CANVAS_END);
		break;

	case GDK_LEAVE_NOTIFY:
		gnome_canvas_configure (canvas, *item,
					GNOME_CANVAS_FILL_COLOR, GNOME_CANVAS_COLOR_STRING, "black",
					GNOME_CANVAS_END);
		break;

	default:
		break;
	}

	return FALSE;
}

void
create_canvas (void)
{
	GtkWidget *app;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *w;
	GtkWidget *frame;
	GtkWidget *canvas;
	GtkAdjustment *adj;
	GnomeCanvasId cid;

	app = create_newwin (TRUE, "testGNOME", "Canvas");

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gnome_app_set_contents (GNOME_APP (app), hbox);
	gtk_widget_show (hbox);

	canvas = gnome_canvas_new (gtk_widget_get_default_visual (), gtk_widget_get_default_colormap ());
	gnome_canvas_set_size (GNOME_CANVAS (canvas), 500, 500);
	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (canvas), 5.0);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0.0, 0.0, 100.0, 100.0);
	gtk_signal_connect (GTK_OBJECT (canvas), "item_event",
			    (GtkSignalFunc) item_event,
			    NULL);

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show (vbox);

	adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 0.1, 1000.0, 0.1, 10.0, 10.0));
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    (GtkSignalFunc) set_pixels_per_unit,
			    canvas);

	w = gtk_spin_button_new (adj, 0.5, 1);
	gtk_widget_set_usize (w, 60, 0);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	gnome_canvas_create_item (GNOME_CANVAS (canvas), "ellipse",
				  20.0, 20.0, 80.0, 80.0,
				  GNOME_CANVAS_FILL_COLOR, GNOME_CANVAS_COLOR_STRING, "red",
				  GNOME_CANVAS_OUTLINE_COLOR, GNOME_CANVAS_COLOR_STRING, "black",
				  GNOME_CANVAS_WIDTH_PIXELS, 10,
				  GNOME_CANVAS_END);

	cid = gnome_canvas_create_item (GNOME_CANVAS (canvas), "rectangle",
					50.0, 35.0, 90.0, 50.0,
					GNOME_CANVAS_FILL_COLOR, GNOME_CANVAS_COLOR_STRING, "green",
					GNOME_CANVAS_END);
	gnome_canvas_bind (GNOME_CANVAS (canvas), cid, GDK_ENTER_NOTIFY, NULL);

	gnome_canvas_create_item (GNOME_CANVAS (canvas), "rectangle",
				  20.0, 65.0, 50.0, 80.0,
				  GNOME_CANVAS_FILL_COLOR, GNOME_CANVAS_COLOR_STRING, "yellow",
				  GNOME_CANVAS_OUTLINE_COLOR, GNOME_CANVAS_COLOR_STRING, "blue",
				  GNOME_CANVAS_WIDTH_PIXELS, 10,
				  GNOME_CANVAS_END);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	gtk_widget_show (app);
}
