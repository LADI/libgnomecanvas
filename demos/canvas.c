#include <config.h>
#include <math.h>
#include "testgnome.h"


#ifdef GTK_HAVE_FEATURES_1_1_0


void
create_canvas (void)
{
	GtkWidget *app;
	GtkWidget *notebook;

/* 	gtk_debug_flags = GTK_DEBUG_OBJECTS; */

	app = create_newwin (TRUE, "testGNOME", "Canvas");

	notebook = gtk_notebook_new ();
	gnome_app_set_contents (GNOME_APP (app), notebook);
	gtk_widget_show (notebook);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_canvas_primitives (), gtk_label_new ("Primitives"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_canvas_arrowhead (), gtk_label_new ("Arrowhead"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), create_canvas_fifteen (), gtk_label_new ("Fifteen"));

	gtk_widget_show (app);
}


#endif
