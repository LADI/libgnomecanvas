/* Editable GnomeCanvas text item based on GtkTextLayout, borrowed heavily
 * from GtkTextView.
 *
 * Copyright (c) 2000 Red Hat, Inc.
 * Copyright (c) 2001 Joe Shaw
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GNOME_CANVAS_RICH_TEXT_H
#define GNOME_CANVAS_RICH_TEXT_H

#include <libgnomecanvas/gnome-canvas.h>
#include <gtk/gtktextlayout.h>

G_BEGIN_DECLS

#define GNOME_TYPE_CANVAS_RICH_TEXT             (gnome_canvas_rich_text_get_type ())
#define GNOME_CANVAS_RICH_TEXT(obj)             (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_RICH_TEXT, GnomeCanvasRichText))
#define GNOME_CANVAS_RICH_TEXT_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_RICH_TEXT, GnomeCanvasRichTextClass))
#define GNOME_IS_CANVAS_RICH_TEXT(obj)          (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_RICH_TEXT))
#define GNOME_IS_CANVAS_RICH_TEXT_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_RICH_TEXT))
#define GNOME_CANVAS_RICH_TEXT_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CANVAS_RICH_TEXT, GnomeCanvasRichTextClass))

typedef struct _GnomeCanvasRichText      GnomeCanvasRichText;
typedef struct _GnomeCanvasRichTextClass GnomeCanvasRichTextClass;

struct _GnomeCanvasRichText {
	GnomeCanvasItem item;

	GtkTextLayout *layout;
	GtkTextBuffer *buffer;

	char *text;

	/* Position at anchor */
	double x, y;
	/* Dimensions */
	double width, height;
	/* Top-left canvas coordinates for text */
	int cx, cy;

	gboolean cursor_visible;
	gboolean cursor_blink;
	gboolean editable;
	gboolean visible;
	gboolean grow_height;
	GtkWrapMode wrap_mode;
	GtkJustification justification;
	GtkTextDirection direction;
	GtkAnchorType anchor;
	int pixels_above_lines;
	int pixels_below_lines;
	int pixels_inside_wrap;
	int left_margin;
	int right_margin;
	int indent;

	guint preblink_timeout;
	guint blink_timeout;

	guint selection_drag_handler;

	gint drag_start_x;
	gint drag_start_y;

	gboolean just_selected_element;

	int clicks;
	guint click_timeout;
};

struct _GnomeCanvasRichTextClass {
	GnomeCanvasItemClass parent_class;

	void (* tag_changed)(GnomeCanvasRichText *text,
			     GtkTextTag *tag);
};

GtkType gnome_canvas_rich_text_get_type(void);

void gnome_canvas_rich_text_cut_clipboard(GnomeCanvasRichText *text);

void gnome_canvas_rich_text_copy_clipboard(GnomeCanvasRichText *text);

void gnome_canvas_rich_text_paste_clipboard(GnomeCanvasRichText *text);

void gnome_canvas_rich_text_set_buffer(GnomeCanvasRichText *text,
				       GtkTextBuffer *buffer);

GtkTextBuffer *gnome_canvas_rich_text_get_buffer(GnomeCanvasRichText *text);

G_END_DECLS

#endif /* GNOME_CANVAS_RICH_TEXT_H */
