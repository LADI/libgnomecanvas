#include <config.h>
#include "testgnome.h"


#ifdef GTK_HAVE_ACCEL_GROUP

static gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	char *color;

	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		color = gtk_object_get_data (GTK_OBJECT (item), "highlight");
		gnome_canvas_item_set (item,
				       "GnomeCanvasRE::fill_color", color,
				       NULL);
		break;

	case GDK_LEAVE_NOTIFY:
		color = gtk_object_get_data (GTK_OBJECT (item), "normal");
		gnome_canvas_item_set (item,
				       "GnomeCanvasRE::fill_color", color,
				       NULL);
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
	GtkWidget *canvas;
	GnomeCanvasGroup *root;
	GnomeCanvasItem *item;
	int x, y;
	char buf[100];
	char *color;

	app = create_newwin (TRUE, "testGNOME", "Canvas");

	canvas = gnome_canvas_new (gtk_widget_get_default_visual (), gtk_widget_get_default_colormap ());
	gnome_canvas_set_size (GNOME_CANVAS (canvas), 500, 500);
	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (canvas), 5.0);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0.0, 0.0, 100.0, 100.0);

	/* Create items */

	root = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (canvas)));

	for (y = 0; y < 100; y += 10)
		for (x = 0; x < 100; x += 10) {
			sprintf (buf, "#%02x%02x80", (x * 255) / 90, (y * 255) / 90);
			color = g_strdup (buf);
			item = gnome_canvas_item_new (GNOME_CANVAS (canvas), root, gnome_canvas_rect_get_type (),
						      "GnomeCanvasRE::x1", (double) x,
						      "GnomeCanvasRE::y1", (double) y,
						      "GnomeCanvasRE::x2", (double) (x + 10),
						      "GnomeCanvasRE::y2", (double) (y + 10),
						      "GnomeCanvasRE::fill_color", color,
						      "GnomeCanvasRE::outline_color", "black",
						      "GnomeCanvasRE::width_pixels", 1,
						      NULL);
			gtk_object_set_data (GTK_OBJECT (item), "normal", color);
			gtk_object_set_data (GTK_OBJECT (item), "highlight", "white");
			gtk_signal_connect (GTK_OBJECT (item), "event",
					    (GtkSignalFunc) item_event,
					    NULL);
		}

	/* Done! */

	gnome_app_set_contents (GNOME_APP (app), canvas);
	gtk_widget_show (canvas);

	gtk_widget_show (app);
}

#endif
