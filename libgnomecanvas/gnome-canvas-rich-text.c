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

#include <math.h>
#include <string.h>

#include "gtk/gtkbindings.h"
#include "gtk/gtkdnd.h"
#include "gtk/gtkmain.h"
#include "gtk/gtkmenu.h"
#include "gtk/gtkmenuitem.h"
#include "gtk/gtkseparatormenuitem.h"
#include "gtk/gtksignal.h"
#include "gtk/gtktextdisplay.h"
#include "gtk/gtktextview.h"
#include "gtk/gtkimmulticontext.h"
#include "gdk/gdkkeysyms.h"
#include <string.h>

#include "libgnomecanvas/gnome-canvas.h"
#include "gtktextcanvas.h"

enum {
	ARG_0,
	ARG_TEXT,
	ARG_X,
	ARG_Y,
	ARG_WIDTH,
	ARG_HEIGHT,
	ARG_EDITABLE,
	ARG_VISIBLE,
	ARG_CURSOR_VISIBLE,
	ARG_CURSOR_BLINK,
	ARG_GROW_HEIGHT,
	ARG_WRAP_MODE,
	ARG_JUSTIFICATION,
	ARG_DIRECTION,
	ARG_ANCHOR,
	ARG_PIXELS_ABOVE_LINES,
	ARG_PIXELS_BELOW_LINES,
	ARG_PIXELS_INSIDE_WRAP,
	ARG_LEFT_MARGIN,
	ARG_RIGHT_MARGIN,
	ARG_INDENT,
};

enum {
	TAG_CHANGED,
	LAST_SIGNAL
};

static GnomeCanvasItemClass *parent_class;
static guint signals[LAST_SIGNAL] = { 0 };

static void gnome_canvas_rich_text_class_init(GnomeCanvasRichTextClass *klass);
static void gnome_canvas_rich_text_init(GnomeCanvasRichText *text);
static void gnome_canvas_rich_text_set_arg(GtkObject *object, GtkArg *arg, 
					   guint arg_id);
static void gnome_canvas_rich_text_get_arg(GtkObject *object, GtkArg *arg, 
					   guint arg_id);
static void gnome_canvas_rich_text_update(GnomeCanvasItem *item, double *affine,
					  ArtSVP *clip_path, int flags);
static void gnome_canvas_rich_text_realize(GnomeCanvasItem *item);
static void gnome_canvas_rich_text_unrealize(GnomeCanvasItem *item);
static double gnome_canvas_rich_text_point(GnomeCanvasItem *item, 
					   double x, double y,
					   int cx, int cy, 
					   GnomeCanvasItem **actual_item);
static void gnome_canvas_rich_text_draw(GnomeCanvasItem *item, 
					GdkDrawable *drawable,
					int x, int y, int width, int height);
static gint gnome_canvas_rich_text_event(GnomeCanvasItem *item, 
					 GdkEvent *event);

static void gnome_canvas_rich_text_ensure_layout(GnomeCanvasRichText *text);
static void gnome_canvas_rich_text_destroy_layout(GnomeCanvasRichText *text);
static void gnome_canvas_rich_text_start_cursor_blink(GnomeCanvasRichText *text, gboolean delay);
static void gnome_canvas_rich_text_stop_cursor_blink(GnomeCanvasRichText *text);
static void gnome_canvas_rich_text_move_cursor(GnomeCanvasRichText *text,
					       GtkMovementStep step,
					       gint count,
					       gboolean extend_selection);
static void gnome_canvas_rich_text_unselect(GnomeCanvasRichText *text);



static GtkTextBuffer *get_buffer(GnomeCanvasRichText *text);
static gint blink_cb(gpointer data);

#define g_signal_handlers_disconnect_by_func(obj, func, data) g_signal_handlers_disconnect_matched(obj, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, func, data)

#define PREBLINK_TIME 300
#define CURSOR_ON_TIME 800
#define CURSOR_OFF_TIME 400

GtkType
gnome_canvas_rich_text_get_type(void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"GnomeCanvasRichText",
			sizeof(GnomeCanvasRichText),
			sizeof(GnomeCanvasRichTextClass),
			(GtkClassInitFunc) gnome_canvas_rich_text_class_init,
			(GtkObjectInitFunc) gnome_canvas_rich_text_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique(gnome_canvas_item_get_type(), &info);
	}

	return type;
} /* gnome_canvas_rich_text_get_type */

static void
gnome_canvas_rich_text_class_init(GnomeCanvasRichTextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	GnomeCanvasItemClass *item_class = GNOME_CANVAS_ITEM_CLASS(klass);
	
	parent_class = gtk_type_class(gnome_canvas_item_get_type());

	gtk_object_add_arg_type(
		"GnomeCanvasRichText::text",
		GTK_TYPE_STRING, GTK_ARG_READWRITE, ARG_TEXT);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::x",
		GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::y",
		GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::width",
		GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_WIDTH);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::height",
		GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_HEIGHT);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::editable",
		GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_EDITABLE);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::visible",
		GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_VISIBLE);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::cursor_visible",
		GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_CURSOR_VISIBLE);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::cursor_blink",
		GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_CURSOR_BLINK);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::grow_height",
		GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_GROW_HEIGHT);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::wrap_mode",
		GTK_TYPE_WRAP_MODE, GTK_ARG_READWRITE, ARG_WRAP_MODE);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::justification",
		GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG_JUSTIFICATION);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::direction",
		GTK_TYPE_DIRECTION_TYPE, GTK_ARG_READWRITE, ARG_DIRECTION);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::anchor",
		GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::pixels_above_lines",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_PIXELS_ABOVE_LINES);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::pixels_below_lines",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_PIXELS_BELOW_LINES);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::pixels_inside_wrap",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_PIXELS_INSIDE_WRAP);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::left_margin",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_LEFT_MARGIN);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::right_margin",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_RIGHT_MARGIN);
	gtk_object_add_arg_type(
		"GnomeCanvasRichText::indent",
		GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_INDENT);

	/* Signals */
	signals[TAG_CHANGED] = g_signal_newc(
		"tag_changed",
		G_OBJECT_CLASS_TYPE(object_class),
		G_SIGNAL_RUN_LAST,
		GTK_SIGNAL_OFFSET(GnomeCanvasRichTextClass, tag_changed),
		NULL, NULL,
		gtk_marshal_VOID__OBJECT,
		G_TYPE_NONE, 1,
		G_TYPE_OBJECT);

	object_class->set_arg = gnome_canvas_rich_text_set_arg;
	object_class->get_arg = gnome_canvas_rich_text_get_arg;

	item_class->update = gnome_canvas_rich_text_update;
	item_class->realize = gnome_canvas_rich_text_realize;
	item_class->unrealize = gnome_canvas_rich_text_unrealize;
	item_class->draw = gnome_canvas_rich_text_draw;
	item_class->point = gnome_canvas_rich_text_point;
#if 0
	item_class->render = gnome_canvas_rich_text_render;
#endif
	item_class->event = gnome_canvas_rich_text_event;
} /* gnome_canvas_rich_text_class_init */

static void
gnome_canvas_rich_text_init(GnomeCanvasRichText *text)
{
	GtkObject *object = GTK_OBJECT(text);

#if 0
	object->flags |= GNOME_CANVAS_ITEM_ALWAYS_REDRAW;
#endif
	/* Try to set some sane defaults */
	text->cursor_visible = TRUE;
	text->cursor_blink = FALSE;
	text->editable = TRUE;
	text->visible = TRUE;
	text->grow_height = FALSE;
	text->wrap_mode = GTK_WRAP_WORD;
	text->justification = GTK_JUSTIFY_LEFT;
	text->direction = gtk_widget_get_default_direction();
	text->anchor = GTK_ANCHOR_NW;
	
	text->blink_timeout = 0;
	text->preblink_timeout = 0;

	text->clicks = 0;
	text->click_timeout = 0;
} /* gnome_canvas_rich_text_init */

static void
gnome_canvas_rich_text_set_arg(GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(object);

	switch (arg_id) {
	case ARG_TEXT:
		if (text->text)
			g_free(text->text);

		text->text = g_strdup(GTK_VALUE_STRING(*arg));

		gtk_text_buffer_set_text(
			get_buffer(text), text->text, strlen(text->text));

		break;
	case ARG_X:
		text->x = GTK_VALUE_DOUBLE(*arg);
		break;
	case ARG_Y:
		text->y = GTK_VALUE_DOUBLE(*arg);
		break;
	case ARG_WIDTH:
		text->width = GTK_VALUE_DOUBLE(*arg);
		break;
	case ARG_HEIGHT:
		text->height = GTK_VALUE_DOUBLE(*arg);
		break;
	case ARG_EDITABLE:
		text->editable = GTK_VALUE_BOOL(*arg);
		if (text->layout) {
			text->layout->default_style->editable =
				text->editable;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_VISIBLE:
		text->visible = GTK_VALUE_BOOL(*arg);
		if (text->layout) {
			text->layout->default_style->invisible =
				!text->visible;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_CURSOR_VISIBLE:
		text->cursor_visible = GTK_VALUE_BOOL(*arg);
		if (text->layout) {
			gtk_text_layout_set_cursor_visible(
				text->layout, text->cursor_visible);

			if (text->cursor_visible && text->cursor_blink) {
				gnome_canvas_rich_text_start_cursor_blink(
					text, FALSE);
			}
			else
				gnome_canvas_rich_text_stop_cursor_blink(text);
		}
		break;
	case ARG_CURSOR_BLINK:
		text->cursor_blink = GTK_VALUE_BOOL(*arg);
		if (text->layout && text->cursor_visible) {
			if (text->cursor_blink && !text->blink_timeout) {
				gnome_canvas_rich_text_start_cursor_blink(
					text, FALSE);
			}
			else if (!text->cursor_blink && text->blink_timeout) {
				gnome_canvas_rich_text_stop_cursor_blink(text);
				gtk_text_layout_set_cursor_visible(
					text->layout, TRUE);
			}
		}
		break;
	case ARG_GROW_HEIGHT:
		text->grow_height = GTK_VALUE_BOOL(*arg);
		/* FIXME: Recalc here */
		break;
	case ARG_WRAP_MODE:
		text->wrap_mode = GTK_VALUE_ENUM(*arg);

		if (text->layout) {
			text->layout->default_style->wrap_mode = 
				text->wrap_mode;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_JUSTIFICATION:
		text->justification = GTK_VALUE_ENUM(*arg);

		if (text->layout) {
			text->layout->default_style->justification =
				text->justification;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_DIRECTION:
		text->direction = GTK_VALUE_ENUM(*arg);

		if (text->layout) {
			text->layout->default_style->direction =
				text->direction;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_ANCHOR:
		text->anchor = GTK_VALUE_ENUM(*arg);
		break;
	case ARG_PIXELS_ABOVE_LINES:
		text->pixels_above_lines = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->pixels_above_lines =
				text->pixels_above_lines;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_PIXELS_BELOW_LINES:
		text->pixels_below_lines = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->pixels_below_lines =
				text->pixels_below_lines;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_PIXELS_INSIDE_WRAP:
		text->pixels_inside_wrap = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->pixels_inside_wrap =
				text->pixels_inside_wrap;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_LEFT_MARGIN:
		text->left_margin = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->left_margin =
				text->left_margin;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_RIGHT_MARGIN:
		text->right_margin = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->right_margin =
				text->right_margin;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
	case ARG_INDENT:
		text->pixels_above_lines = GTK_VALUE_INT(*arg);
		
		if (text->layout) {
			text->layout->default_style->indent = text->indent;
			gtk_text_layout_default_style_changed(text->layout);
		}
		break;
		       
	default:
		break;
	}

	gnome_canvas_item_request_update(GNOME_CANVAS_ITEM(text));
} /* gnome_canvas_rich_text_set_arg */

static void
gnome_canvas_rich_text_get_arg(GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(object);

	switch (arg_id) {
	case ARG_TEXT:
		GTK_VALUE_STRING(*arg) = g_strdup(text->text);
		break;
	case ARG_X:
		GTK_VALUE_DOUBLE(*arg) = text->x;
		break;
	case ARG_Y:
		GTK_VALUE_DOUBLE(*arg) = text->y;
		break;
	case ARG_EDITABLE:
		GTK_VALUE_BOOL(*arg) = text->editable;
		break;
	case ARG_CURSOR_VISIBLE:
		GTK_VALUE_BOOL(*arg) = text->cursor_visible;
		break;
	case ARG_CURSOR_BLINK:
		GTK_VALUE_BOOL(*arg) = text->cursor_blink;
		break;
	case ARG_GROW_HEIGHT:
		GTK_VALUE_BOOL(*arg) = text->grow_height;
		break;
	case ARG_WRAP_MODE:
		GTK_VALUE_ENUM(*arg) = text->wrap_mode;
		break;
	case ARG_JUSTIFICATION:
		GTK_VALUE_ENUM(*arg) = text->justification;
		break;
	case ARG_DIRECTION:
		GTK_VALUE_ENUM(*arg) = text->direction;
		break;
	case ARG_ANCHOR:
		GTK_VALUE_ENUM(*arg) = text->anchor;
		break;
	case ARG_PIXELS_ABOVE_LINES:
		GTK_VALUE_INT(*arg) = text->pixels_above_lines;
		break;
	case ARG_PIXELS_BELOW_LINES:
		GTK_VALUE_INT(*arg) = text->pixels_below_lines;
		break;
	case ARG_PIXELS_INSIDE_WRAP:
		GTK_VALUE_INT(*arg) = text->pixels_inside_wrap;
		break;
	case ARG_LEFT_MARGIN:
		GTK_VALUE_INT(*arg) = text->left_margin;
		break;
	case ARG_RIGHT_MARGIN:
		GTK_VALUE_INT(*arg) = text->right_margin;
		break;
	case ARG_INDENT:
		GTK_VALUE_INT(*arg) = text->indent;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
} /* gnome_canvas_rich_text_get_arg */

static void
gnome_canvas_rich_text_realize(GnomeCanvasItem *item)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);

	(* GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)(item);

	gnome_canvas_rich_text_ensure_layout(text);
} /* gnome_canvas_rich_text_realize */

static void
gnome_canvas_rich_text_unrealize(GnomeCanvasItem *item)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);

	gnome_canvas_rich_text_destroy_layout(text);

	(* GNOME_CANVAS_ITEM_CLASS(parent_class)->unrealize)(item);
} /* gnome_canvas_rich_text_unrealize */

static void
gnome_canvas_rich_text_move_iter_by_lines(GnomeCanvasRichText *text,
					  GtkTextIter *newplace, gint count)
{
	while (count < 0) {
		gtk_text_layout_move_iter_to_previous_line(
			text->layout, newplace);
		count++;
	}

	while (count > 0) {
		gtk_text_layout_move_iter_to_next_line(
			text->layout, newplace);
		count--;
	}
} /* gnome_canvas_rich_text_move_iter_by_lines */

static gint
gnome_canvas_rich_text_get_cursor_x_position(GnomeCanvasRichText *text)
{
	GtkTextIter insert;
	GdkRectangle rect;

	gtk_text_buffer_get_iter_at_mark(
		get_buffer(text), &insert,
		gtk_text_buffer_get_mark(get_buffer(text), "insert"));
	gtk_text_layout_get_cursor_locations(
		text->layout, &insert, &rect, NULL);

	return rect.x;
} /* gnome_canvas_rich_text_get_cursor_x_position */

static void
gnome_canvas_rich_text_move_cursor(GnomeCanvasRichText *text,
				   GtkMovementStep step,
				   gint count, gboolean extend_selection)
{
	GtkTextIter insert, newplace;
	int cursor_x_pos = 0;

	gtk_text_buffer_get_iter_at_mark(
		get_buffer(text), &insert, 
		gtk_text_buffer_get_mark(get_buffer(text), "insert"));

	newplace = insert;

	switch (step) {
	case GTK_MOVEMENT_LOGICAL_POSITIONS:
		gtk_text_iter_forward_cursor_positions(&newplace, count);
		break;
	case GTK_MOVEMENT_VISUAL_POSITIONS:
		gtk_text_layout_move_iter_visually(
			text->layout, &newplace, count);
		break;
	case GTK_MOVEMENT_WORDS:
		if (count < 0)
			gtk_text_iter_backward_word_starts(&newplace, -count);
		else if (count > 0)
			gtk_text_iter_forward_word_ends(&newplace, count);
		break;
	case GTK_MOVEMENT_DISPLAY_LINES:
		gnome_canvas_rich_text_move_iter_by_lines(
			text, &newplace, count);
		gtk_text_layout_move_iter_to_x(
			text->layout, &newplace, 
			gnome_canvas_rich_text_get_cursor_x_position(text));
		break;
	case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
		if (count > 1) {
			gnome_canvas_rich_text_move_iter_by_lines(
				text, &newplace, --count);
		}
		else if (count < -1) {
			gnome_canvas_rich_text_move_iter_by_lines(
				text, &newplace, ++count);
		}
	       
		if (count != 0) {
			gtk_text_layout_move_iter_to_line_end(
				text->layout, &newplace, count);
		}
		break;
	case GTK_MOVEMENT_PARAGRAPHS:
		/* FIXME: Busted in gtktextview.c too */
		break;
	case GTK_MOVEMENT_PARAGRAPH_ENDS:
		if (count > 0)
			gtk_text_iter_forward_to_line_end(&newplace);
		else if (count < 0)
			gtk_text_iter_set_line_offset(&newplace, 0);
		break;
	case GTK_MOVEMENT_BUFFER_ENDS:
		if (count > 0) {
			gtk_text_buffer_get_end_iter(
				get_buffer(text), &newplace);
		}
		else if (count < 0) {
			gtk_text_buffer_get_iter_at_offset(
				get_buffer(text), &newplace, 0);
		}
		break;
	default:
		break;
	}

	if (!gtk_text_iter_equal(&insert, &newplace)) {
		if (extend_selection) {
			gtk_text_buffer_move_mark(
				get_buffer(text),
				gtk_text_buffer_get_mark(
					get_buffer(text), "insert"),
				&newplace);
		}
		else {
			gtk_text_buffer_place_cursor(
				get_buffer(text), &newplace);
		}
	}

	gnome_canvas_rich_text_start_cursor_blink(text, TRUE);
} /* gnome_canvas_rich_text_move_cursor */

static gboolean
whitespace(gunichar ch, gpointer user_data)
{
	return (ch == ' ' || ch == '\t');
} /* whitespace */

static gboolean
not_whitespace(gunichar ch, gpointer user_data)
{
	return !whitespace(ch, user_data);
} /* not_whitespace */

static gboolean
find_whitespace_region(const GtkTextIter *center,
		       GtkTextIter *start, GtkTextIter *end)
{
	*start = *center;
	*end = *center;

	if (gtk_text_iter_backward_find_char(start, not_whitespace, NULL, NULL))
		gtk_text_iter_forward_char(start);
	if (whitespace(gtk_text_iter_get_char(end), NULL))
		gtk_text_iter_forward_find_char(end, not_whitespace, NULL, NULL);

	return !gtk_text_iter_equal(start, end);
} /* find_whitespace_region */

static void
gnome_canvas_rich_text_delete_from_cursor(GnomeCanvasRichText *text,
					  GtkDeleteType type,
					  gint count)
{
	GtkTextIter insert, start, end;

	/* Special case: If the user wants to delete a character and there is
	   a selection, then delete the selection and return */
	if (type == GTK_DELETE_CHARS) {
		if (gtk_text_buffer_delete_selection(get_buffer(text), TRUE, 
						     text->editable))
			return;
	}

	gtk_text_buffer_get_iter_at_mark(
		get_buffer(text), &insert,
		gtk_text_buffer_get_mark(get_buffer(text), "insert"));

	start = insert;
	end = insert;

	switch (type) {
	case GTK_DELETE_CHARS:
		gtk_text_iter_forward_cursor_positions(&end, count);
		break;
	case GTK_DELETE_WORD_ENDS:
		if (count > 0)
			gtk_text_iter_forward_word_ends(&end, count);
		else if (count < 0)
			gtk_text_iter_backward_word_starts(&start, -count);
		break;
	case GTK_DELETE_WORDS:
		break;
	case GTK_DELETE_DISPLAY_LINE_ENDS:
		break;
	case GTK_DELETE_PARAGRAPH_ENDS:
		if (gtk_text_iter_ends_line(&end)) {
			gtk_text_iter_forward_line(&end);
			--count;
		}

		while (count > 0) {
			if (!gtk_text_iter_forward_to_line_end(&end))
				break;

			--count;
		}
		break;
	case GTK_DELETE_PARAGRAPHS:
		if (count > 0) {
			gtk_text_iter_set_line_offset(&start, 0);
			gtk_text_iter_forward_to_line_end(&end);

			/* Do the lines beyond the first. */
			while (count > 1) {
				gtk_text_iter_forward_to_line_end(&end);
				--count;
			}
		}
		break;
	case GTK_DELETE_WHITESPACE:
		find_whitespace_region(&insert, &start, &end);
		break;

	default:
		break;
	}

	if (!gtk_text_iter_equal(&start, &end)) {
		gtk_text_buffer_begin_user_action(get_buffer(text));
		gtk_text_buffer_delete_interactive(
			get_buffer(text), &start, &end, text->editable);
		gtk_text_buffer_end_user_action(get_buffer(text));
	}
} /* gnome_canvas_rich_text_delete_from_cursor */

static gint
selection_motion_event_handler(GnomeCanvasRichText *text, GdkEvent *event,
			       gpointer data)
{
	GtkTextIter newplace;
	GtkTextMark *mark;
	double newx, newy;

	/* We only want to handle motion events... */
	if (event->type != GDK_MOTION_NOTIFY)
		return FALSE;

	newx = (event->motion.x - text->x) * 
		GNOME_CANVAS_ITEM(text)->canvas->pixels_per_unit;
	newy = (event->motion.y - text->y) * 
		GNOME_CANVAS_ITEM(text)->canvas->pixels_per_unit;

	gtk_text_layout_get_iter_at_pixel(text->layout, &newplace, newx, newy);
	mark = gtk_text_buffer_get_mark(get_buffer(text), "insert");
	gtk_text_buffer_move_mark(get_buffer(text), mark, &newplace);

	return TRUE;
} /* selection_motion_event_handler */

static void
gnome_canvas_rich_text_start_selection_drag(GnomeCanvasRichText *text,
					    const GtkTextIter *iter,
					    GdkEventButton *button)
{
	GtkTextIter newplace;

	g_return_if_fail(text->selection_drag_handler == 0);

#if 0
	gnome_canvas_item_grab_focus(GNOME_CANVAS_ITEM(text));
#endif

	newplace = *iter;

	gtk_text_buffer_place_cursor(get_buffer(text), &newplace);

	text->selection_drag_handler = gtk_signal_connect(
		GTK_OBJECT(text), "event",
		GTK_SIGNAL_FUNC(selection_motion_event_handler), NULL);
} /* gnome_canvas_rich_text_start_selection_drag */

static gboolean
gnome_canvas_rich_text_end_selection_drag(GnomeCanvasRichText *text,
					  GdkEventButton *event)
{
	if (text->selection_drag_handler == 0)
		return FALSE;

	gtk_signal_disconnect(GTK_OBJECT(text), text->selection_drag_handler);
	text->selection_drag_handler = 0;

#if 0
	gnome_canvas_item_grab(NULL);
#endif

	return TRUE;
} /* gnome_canvas_rich_text_end_selection_drag */

static void
gnome_canvas_rich_text_emit_tag_changed(GnomeCanvasRichText *text,
					GtkTextTag *tag)
{
	g_signal_emit(G_OBJECT(text), signals[TAG_CHANGED], 0, tag);
} /* gnome_canvas_rich_text_emit_tag_changed */
						
static gint
gnome_canvas_rich_text_key_press_event(GnomeCanvasItem *item, 
				       GdkEventKey *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	gboolean extend_selection = FALSE;
	gboolean handled = FALSE;

#if 0
	printf("Key press event\n");
#endif

	if (!text->layout || !text->buffer)
		return FALSE;

	if (event->state & GDK_SHIFT_MASK)
		extend_selection = TRUE;

	switch (event->keyval) {
	case GDK_Return:
	case GDK_KP_Enter:
		gtk_text_buffer_delete_selection(
			get_buffer(text), TRUE, text->editable);
		gtk_text_buffer_insert_interactive_at_cursor(
			get_buffer(text), "\n", 1, text->editable);
		handled = TRUE;
		break;

	case GDK_Tab:
		gtk_text_buffer_insert_interactive_at_cursor(
			get_buffer(text), "\t", 1, text->editable);
		handled = TRUE;
		break;

	/* MOVEMENT */
	case GDK_Right:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_WORDS, 1, 
				extend_selection);
			handled = TRUE;
		}
		else {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_VISUAL_POSITIONS, 1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_Left:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_WORDS, -1,
				extend_selection);
			handled = TRUE;
		}
		else {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_VISUAL_POSITIONS, -1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_f:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_LOGICAL_POSITIONS, 1,
				extend_selection);
			handled = TRUE;
		}
		else if (event->state & GDK_MOD1_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_WORDS, 1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_b:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_LOGICAL_POSITIONS, -1,
				extend_selection);
			handled = TRUE;
		}
		else if (event->state & GDK_MOD1_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_WORDS, -1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_Up:
		gnome_canvas_rich_text_move_cursor(
			text, GTK_MOVEMENT_DISPLAY_LINES, -1,
			extend_selection);
		handled = TRUE;
		break;
	case GDK_Down:
		gnome_canvas_rich_text_move_cursor(
			text, GTK_MOVEMENT_DISPLAY_LINES, 1,
			extend_selection);
		handled = TRUE;
		break;
	case GDK_p:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_DISPLAY_LINES, -1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_n:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_DISPLAY_LINES, 1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_Home:
		gnome_canvas_rich_text_move_cursor(
			text, GTK_MOVEMENT_PARAGRAPH_ENDS, -1,
			extend_selection);
		handled = TRUE;
		break;
	case GDK_End:
		gnome_canvas_rich_text_move_cursor(
			text, GTK_MOVEMENT_PARAGRAPH_ENDS, 1,
			extend_selection);
		handled = TRUE;
		break;
	case GDK_a:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_PARAGRAPH_ENDS, -1,
				extend_selection);
			handled = TRUE;
		}
		break;
	case GDK_e:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_move_cursor(
				text, GTK_MOVEMENT_PARAGRAPH_ENDS, 1,
				extend_selection);
			handled = TRUE;
		}
		break;

	/* DELETING TEXT */
	case GDK_Delete:
	case GDK_KP_Delete:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_WORD_ENDS, 1);
			handled = TRUE;
		}
		else {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_CHARS, 1);
			handled = TRUE;
		}
		break;
	case GDK_d:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_CHARS, 1);
			handled = TRUE;
		}
		else if (event->state & GDK_MOD1_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_WORD_ENDS, 1);
			handled = TRUE;
		}
		break;
	case GDK_BackSpace:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_WORD_ENDS, -1);
			handled = TRUE;
		}
		else {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_CHARS, -1);
		}
		handled = TRUE;
		break;
	case GDK_k:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_PARAGRAPH_ENDS, 1);
			handled = TRUE;
		}
		break;
	case GDK_u:
		if (event->state & GDK_CONTROL_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_PARAGRAPHS, 1);
			handled = TRUE;
		}
		break;
	case GDK_space:
		if (event->state & GDK_MOD1_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_WHITESPACE, 1);
			handled = TRUE;
		}
		break;
	case GDK_backslash:
		if (event->state & GDK_MOD1_MASK) {
			gnome_canvas_rich_text_delete_from_cursor(
				text, GTK_DELETE_WHITESPACE, 1);
			handled = TRUE;
		}
		break;
	default:
		break;
	}

	/* An empty string, click just pressing "Alt" by itself or whatever. */
	if (!event->length)
		return FALSE;

	if (!handled) {
		gtk_text_buffer_delete_selection(
			get_buffer(text), TRUE, text->editable);
		gtk_text_buffer_insert_interactive_at_cursor(
			get_buffer(text), event->string, event->length,
			text->editable);
	}

	gnome_canvas_rich_text_start_cursor_blink(text, TRUE);
	
	return TRUE;
} /* gnome_canvas_rich_text_key_press_event */

static gint
gnome_canvas_rich_text_key_release_event(GnomeCanvasItem *item, 
					 GdkEventKey *event)
{
	return FALSE;
} /* gnome_canvas_rich_text_key_release_event */

static gboolean
_click(gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);

	text->clicks = 0;
	text->click_timeout = 0;

	return FALSE;
} /* _click */

static gint
gnome_canvas_rich_text_button_press_event(GnomeCanvasItem *item,
					  GdkEventButton *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	GtkTextIter iter;
	GdkEventType event_type;
	double newx, newy;

	newx = (event->x - text->x) * item->canvas->pixels_per_unit;
	newy = (event->y - text->y) * item->canvas->pixels_per_unit;
	
	gtk_text_layout_get_iter_at_pixel(text->layout, &iter, newx, newy);

	/* The canvas doesn't give us double- or triple-click events, so
	   we have to synthesize them ourselves. Yay. */
	if (event->type == GDK_BUTTON_PRESS) {
		text->clicks++;
		text->click_timeout = gtk_timeout_add(400, _click, text);

		if (text->clicks > 3)
			text->clicks = text->clicks % 3;

		if (text->clicks == 1)
			event_type = GDK_BUTTON_PRESS;
		else if (text->clicks == 2)
			event_type = GDK_2BUTTON_PRESS;
		else if (text->clicks == 3)
			event_type = GDK_3BUTTON_PRESS;
		else
			printf("ZERO CLICKS!\n");
	}

	if (event->button == 1 && event_type == GDK_BUTTON_PRESS) {
		GtkTextIter start, end;

		if (gtk_text_buffer_get_selection_bounds(
			    get_buffer(text), &start, &end) &&
		    gtk_text_iter_in_range(&iter, &start, &end)) {
			text->drag_start_x = event->x;
			text->drag_start_y = event->y;
		}
		else {
			gnome_canvas_rich_text_start_selection_drag(
				text, &iter, event);
		}

		return TRUE;
	}
	else if (event->button == 1 && event_type == GDK_2BUTTON_PRESS) {
		GtkTextIter start, end;

		printf("double-click\n");
		
		gnome_canvas_rich_text_end_selection_drag(text, event);

		start = iter;
		end = start;

		if (gtk_text_iter_inside_word(&start)) {
			if (!gtk_text_iter_starts_word(&start))
				gtk_text_iter_backward_word_start(&start);
			
			if (!gtk_text_iter_ends_word(&end))
				gtk_text_iter_forward_word_end(&end);
		}

		gtk_text_buffer_move_mark(
			get_buffer(text),
			gtk_text_buffer_get_selection_bound(get_buffer(text)),
			&start);
		gtk_text_buffer_move_mark(
			get_buffer(text),
			gtk_text_buffer_get_insert(get_buffer(text)), &end);

		text->just_selected_element = TRUE;

		return TRUE;
	}
	else if (event->button == 1 && event_type == GDK_3BUTTON_PRESS) {
		GtkTextIter start, end;

		printf("triple-click\n");

		gnome_canvas_rich_text_end_selection_drag(text, event);

		start = iter;
		end = start;

		if (gtk_text_layout_iter_starts_line(text->layout, &start)) {
			gtk_text_layout_move_iter_to_line_end(
				text->layout, &start, -1);
		}
		else {
			gtk_text_layout_move_iter_to_line_end(
				text->layout, &start, -1);

			if (!gtk_text_layout_iter_starts_line(
				    text->layout, &end)) {
				gtk_text_layout_move_iter_to_line_end(
					text->layout, &end, 1);
			}
		}

		gtk_text_buffer_move_mark(
			get_buffer(text),
			gtk_text_buffer_get_selection_bound(get_buffer(text)),
			&start);
		gtk_text_buffer_move_mark(
			get_buffer(text),
			gtk_text_buffer_get_insert(get_buffer(text)), &end);

		text->just_selected_element = TRUE;

		return TRUE;
	}
	else if (event->button == 2 && event_type == GDK_BUTTON_PRESS) {
		gtk_text_buffer_paste_primary(
			get_buffer(text), &iter, text->editable);
	}
		
	return FALSE;
} /* gnome_canvas_rich_text_button_press_event */

static gint
gnome_canvas_rich_text_button_release_event(GnomeCanvasItem *item,
					    GdkEventButton *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	double newx, newy;

	newx = (event->x - text->x) * item->canvas->pixels_per_unit;
	newy = (event->y - text->y) * item->canvas->pixels_per_unit;
	
	if (event->button == 1) {
		if (text->drag_start_x >= 0) {
			text->drag_start_x = -1;
			text->drag_start_y = -1;
		}

		if (gnome_canvas_rich_text_end_selection_drag(text, event))
			return TRUE;
		else if (text->just_selected_element) {
			text->just_selected_element = FALSE;
			return FALSE;
		}
		else {
			GtkTextIter iter;

			gtk_text_layout_get_iter_at_pixel(
				text->layout, &iter, newx, newy);

			gtk_text_buffer_place_cursor(get_buffer(text), &iter);

			return FALSE;
		}
	}

	return FALSE;
} /* gnome_canvas_rich_text_button_release_event */

static gint
gnome_canvas_rich_text_focus_in_event(GnomeCanvasItem *item,
				      GdkEventFocus *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);

	if (text->cursor_visible && text->layout) {
		gtk_text_layout_set_cursor_visible(text->layout, TRUE);
		gnome_canvas_rich_text_start_cursor_blink(text, FALSE);
	}

	return FALSE;
} /* gnome_canvas_rich_text_focus_in_event */

static gint
gnome_canvas_rich_text_focus_out_event(GnomeCanvasItem *item,
				       GdkEventFocus *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);

	if (text->cursor_visible && text->layout) {
		gtk_text_layout_set_cursor_visible(text->layout, FALSE);
		gnome_canvas_rich_text_stop_cursor_blink(text);
	}

	return FALSE;
} /* gnome_canvas_rich_text_focus_out_event */

static gboolean
get_event_coordinates(GdkEvent *event, gint *x, gint *y)
{
	g_return_val_if_fail(event, FALSE);
	
	switch (event->type) {
	case GDK_MOTION_NOTIFY:
		*x = event->motion.x;
		*y = event->motion.y;
		return TRUE;
		break;
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		*x = event->button.x;
		*y = event->button.y;
		return TRUE;
		break;

	default:
		return FALSE;
		break;
	}
} /* get_event_coordinates */

static void
emit_event_on_tags(GnomeCanvasRichText *text, GdkEvent *event, 
		   GtkTextIter *iter)
{
	GSList *tags;
	GSList *i;
	
	tags = gtk_text_iter_get_tags(iter);

	i = tags;
	while (i) {
		GtkTextTag *tag = i->data;

		gtk_text_tag_event(tag, G_OBJECT(text), event, iter);

		/* The cursor has been moved to within this tag. Emit the
		   tag_changed signal */
		if (event->type == GDK_BUTTON_RELEASE || 
		    event->type == GDK_KEY_PRESS ||
		    event->type == GDK_KEY_RELEASE) {
			gnome_canvas_rich_text_emit_tag_changed(
				text, tag);
		}

		i = g_slist_next(i);
	}

	g_slist_free(tags);
} /* emit_event_on_tags */

static gint
gnome_canvas_rich_text_event(GnomeCanvasItem *item, GdkEvent *event)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	int x, y;

	if (get_event_coordinates(event, &x, &y)) {
		GtkTextIter iter;

		x -= text->x;
		y -= text->y;

		gtk_text_layout_get_iter_at_pixel(text->layout, &iter, x, y);
		emit_event_on_tags(text, event, &iter);
	}
	else if (event->type == GDK_KEY_PRESS ||
		 event->type == GDK_KEY_RELEASE) {
		GtkTextMark *insert;
		GtkTextIter iter;

		insert = gtk_text_buffer_get_mark(get_buffer(text), "insert");
		gtk_text_buffer_get_iter_at_mark(
			get_buffer(text), &iter, insert);
		emit_event_on_tags(text, event, &iter);
	}

	switch (event->type) {
	case GDK_KEY_PRESS:
		return gnome_canvas_rich_text_key_press_event(
			item, (GdkEventKey *) event);
	case GDK_KEY_RELEASE:
		return gnome_canvas_rich_text_key_release_event(
			item, (GdkEventKey *) event);
	case GDK_BUTTON_PRESS:
		return gnome_canvas_rich_text_button_press_event(
			item, (GdkEventButton *) event);
	case GDK_BUTTON_RELEASE:
		return gnome_canvas_rich_text_button_release_event(
			item, (GdkEventButton *) event);
	case GDK_FOCUS_CHANGE:
		if (((GdkEventFocus *) event)->window !=
		    item->canvas->layout.bin_window)
			return FALSE;

		if (((GdkEventFocus *) event)->in)
			return gnome_canvas_rich_text_focus_in_event(
				item, (GdkEventFocus *) event);
		else
			return gnome_canvas_rich_text_focus_out_event(
				item, (GdkEventFocus *) event);
	default:
		return FALSE;
	}
} /* gnome_canvas_rich_text_event */

/* Cut/Copy/Paste */
void
gnome_canvas_rich_text_cut_clipboard(GnomeCanvasRichText *text)
{
	g_return_if_fail(text);
	g_return_if_fail(get_buffer(text));

	gtk_text_buffer_cut_clipboard(get_buffer(text), text->editable);
} /* gnome_canvas_rich_text_cut_clipboard */

void
gnome_canvas_rich_text_copy_clipboard(GnomeCanvasRichText *text)
{
	g_return_if_fail(text);
	g_return_if_fail(get_buffer(text));

	gtk_text_buffer_copy_clipboard(get_buffer(text));
} /* gnome_canvas_rich_text_cut_clipboard */

void
gnome_canvas_rich_text_paste_clipboard(GnomeCanvasRichText *text)
{
	g_return_if_fail(text);
	g_return_if_fail(get_buffer(text));

	gtk_text_buffer_paste_clipboard(get_buffer(text), text->editable);
} /* gnome_canvas_rich_text_cut_clipboard */

static gint
preblink_cb(gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);

	text->preblink_timeout = 0;
	gnome_canvas_rich_text_start_cursor_blink(text, FALSE);

	/* Remove ourselves */
	return FALSE;
} /* preblink_cb */

static gint
blink_cb(gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);
	gboolean visible;

	g_assert(text->layout);
	g_assert(text->cursor_visible);

	visible = gtk_text_layout_get_cursor_visible(text->layout);
	if (visible)
		text->blink_timeout = gtk_timeout_add(
			CURSOR_OFF_TIME, blink_cb, text);
	else
		text->blink_timeout = gtk_timeout_add(
			CURSOR_ON_TIME, blink_cb, text);

	gtk_text_layout_set_cursor_visible(text->layout, !visible);

	/* Remove ourself */
	return FALSE;
} /* blink_cb */

static void
gnome_canvas_rich_text_start_cursor_blink(GnomeCanvasRichText *text,
					  gboolean with_delay)
{
	if (!text->layout)
		return;

	if (!text->cursor_visible || !text->cursor_blink)
		return;

	if (text->preblink_timeout != 0) {
		gtk_timeout_remove(text->preblink_timeout);
		text->preblink_timeout = 0;
	}

	if (with_delay) {
		if (text->blink_timeout != 0) {
			gtk_timeout_remove(text->blink_timeout);
			text->blink_timeout = 0;
		}

		gtk_text_layout_set_cursor_visible(text->layout, TRUE);

		text->preblink_timeout = gtk_timeout_add(
			PREBLINK_TIME, preblink_cb, text);
	}
	else {
		if (text->blink_timeout == 0) {
			gtk_text_layout_set_cursor_visible(text->layout, TRUE);
			text->blink_timeout = gtk_timeout_add(
				CURSOR_ON_TIME, blink_cb, text);
		}
	}
} /* gnome_canvas_rich_text_start_cursor_blink */

static void
gnome_canvas_rich_text_stop_cursor_blink(GnomeCanvasRichText *text)
{
	if (text->blink_timeout) {
		gtk_timeout_remove(text->blink_timeout);
		text->blink_timeout = 0;
	}
} /* gnome_canvas_rich_text_stop_cursor_blink */

/* We have to request updates this way because the update cycle is not
   re-entrant. This will fire off a request in an idle loop. */
static gboolean
request_update(gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);

	gnome_canvas_item_request_update(GNOME_CANVAS_ITEM(text));

	return FALSE;
} /* request_update */

static void
invalidated_handler(GtkTextLayout *layout, gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);
	GtkTextIter start;

#if 0
	printf("Text is being invalidated.\n");
#endif

	gtk_text_layout_validate(text->layout, 2000);

	/* We are called from the update cycle; gotta put this in an idle
	   loop. */
	gtk_idle_add(request_update, text);
} /* invalidated_handler */

static void
scale_fonts(GtkTextTag *tag, gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);
	GtkTextAttributes *attr;

	if (!tag->values)
		return;

	g_object_set(
		G_OBJECT(tag), "scale", 
		text->layout->default_style->font_scale, NULL);
} /* scale_fonts */

static void
changed_handler(GtkTextLayout *layout, gint start_y, 
		gint old_height, gint new_height, gpointer data)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(data);

#if 0
	printf("Layout %p is being changed.\n", text->layout);
#endif

	if (text->layout->default_style->font_scale != 
	    GNOME_CANVAS_ITEM(text)->canvas->pixels_per_unit) {
		GtkTextTagTable *tag_table;

		text->layout->default_style->font_scale = 
			GNOME_CANVAS_ITEM(text)->canvas->pixels_per_unit;

		tag_table = gtk_text_buffer_get_tag_table(get_buffer(text));
		gtk_text_tag_table_foreach(tag_table, scale_fonts, text);

		gtk_text_layout_default_style_changed(text->layout);
	}

	if (text->grow_height) {
		int width, height;

		gtk_text_layout_get_size(text->layout, &width, &height);

		if (height > text->height)
			text->height = height;
	}

	/* We are called from the update cycle; gotta put this in an idle
	   loop. */
	gtk_idle_add(request_update, text);
} /* changed_handler */

void
gnome_canvas_rich_text_set_buffer(GnomeCanvasRichText *text, 
				  GtkTextBuffer *buffer)
{
	g_return_if_fail(GNOME_IS_CANVAS_RICH_TEXT(text));
	g_return_if_fail(buffer == NULL || GTK_IS_TEXT_BUFFER(buffer));

	if (text->buffer == buffer)
		return;

	if (text->buffer != NULL) {
		g_object_unref(G_OBJECT(text->buffer));
	}

	text->buffer = buffer;

	if (buffer) {
		g_object_ref(G_OBJECT(buffer));

		if (text->layout)
			gtk_text_layout_set_buffer(text->layout, buffer);
	}

	gnome_canvas_item_request_update(GNOME_CANVAS_ITEM(text));
} /* gnome_canvas_rich_text_set_buffer */

static GtkTextBuffer *
get_buffer(GnomeCanvasRichText *text)
{
	if (!text->buffer) {
		GtkTextBuffer *b;

		b = gtk_text_buffer_new(NULL);
		gnome_canvas_rich_text_set_buffer(text, b);
		g_object_unref(G_OBJECT(b));
	}

	return text->buffer;
} /* get_buffer */

GtkTextBuffer *
gnome_canvas_rich_text_get_buffer(GnomeCanvasRichText *text)
{
	g_return_val_if_fail(GNOME_IS_CANVAS_RICH_TEXT(text), NULL);

	return get_buffer(text);
} /* gnome_canvas_rich_text_get_buffer */

static void
gnome_canvas_rich_text_set_attributes_from_style(GnomeCanvasRichText *text,
						 GtkTextAttributes *values,
						 GtkStyle *style)
{
	values->appearance.bg_color = style->base[GTK_STATE_NORMAL];
	values->appearance.fg_color = style->fg[GTK_STATE_NORMAL];
	
	if (values->font.family_name)
		g_free (values->font.family_name);
	
	values->font = *style->font_desc;
	values->font.family_name = g_strdup (style->font_desc->family_name);
} /* gnome_canvas_rich_text_set_attributes_from_style */

static void
gnome_canvas_rich_text_ensure_layout(GnomeCanvasRichText *text)
{
	if (!text->layout) {
		GtkWidget *canvas;
		GtkTextAttributes *style;
		PangoContext *ltr_context, *rtl_context;
		GSList *tmp_list;

		text->layout = gtk_text_layout_new();

		gtk_text_layout_set_screen_width(text->layout, text->width);

		if (get_buffer(text)) {
			gtk_text_layout_set_buffer(
				text->layout, get_buffer(text));
		}

		/* Setup the cursor stuff */
		gtk_text_layout_set_cursor_visible(
			text->layout, text->cursor_visible);
		if (text->cursor_visible && text->cursor_blink)
			gnome_canvas_rich_text_start_cursor_blink(text, FALSE);
		else
			gnome_canvas_rich_text_stop_cursor_blink(text);

		canvas = GTK_WIDGET(GNOME_CANVAS_ITEM(text)->canvas);

		ltr_context = gtk_widget_create_pango_context(canvas);
		pango_context_set_base_dir(ltr_context, PANGO_DIRECTION_LTR);
		rtl_context = gtk_widget_create_pango_context(canvas);
		pango_context_set_base_dir(rtl_context, PANGO_DIRECTION_RTL);

		gtk_text_layout_set_contexts(
			text->layout, ltr_context, rtl_context);

		g_object_unref(G_OBJECT(ltr_context));
		g_object_unref(G_OBJECT(rtl_context));

		style = gtk_text_attributes_new();

		gnome_canvas_rich_text_set_attributes_from_style(
			text, style, canvas->style);

		style->pixels_above_lines = text->pixels_above_lines;
		style->pixels_below_lines = text->pixels_below_lines;
		style->pixels_inside_wrap = text->pixels_inside_wrap;
		style->left_margin = text->left_margin;
		style->right_margin = text->right_margin;
		style->indent = text->indent;
		style->tabs = NULL;
		style->wrap_mode = text->wrap_mode;
		style->justification = text->justification;
		style->direction = text->direction;
		style->editable = text->editable;
		style->invisible = !text->visible;

		gtk_text_layout_set_default_style(text->layout, style);

		gtk_text_attributes_unref(style);

		g_signal_connect_data(
			G_OBJECT(text->layout), "invalidated",
			invalidated_handler, text, NULL, FALSE, FALSE);

		g_signal_connect_data(
			G_OBJECT(text->layout), "changed",
			changed_handler, text, NULL, FALSE, FALSE);

#if 0
		g_signal_connect_data(
			G_OBJECT(text->layout), "allocate_child",
			child_allocated, text, NULL, FALSE, FALSE);
#endif
	}
} /* gnome_canvas_rich_text_ensure_layout */

static void
gnome_canvas_rich_text_destroy_layout(GnomeCanvasRichText *text)
{
	if (text->layout) {
		g_signal_handlers_disconnect_by_func(
			G_OBJECT(text->layout), invalidated_handler, text);
		g_signal_handlers_disconnect_by_func(
			G_OBJECT(text->layout), changed_handler, text);
		g_object_unref(G_OBJECT(text->layout));
		text->layout = NULL;
	}
} /* gnome_canvas_rich_text_destroy_layout */

static void
adjust_for_anchors(GnomeCanvasRichText *text, double *ax, double *ay)
{
	double x, y;

	x = text->x;
	y = text->y;

	/* Anchor text */
	/* X coordinates */
	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->width;
		break;
	}

	/* Y coordinates */
	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		break;
	}

	if (ax)
		*ax = x;
	if (ay)
		*ay = y;
} /* adjust_for_anchors */

static void
get_bounds(GnomeCanvasRichText *text, double *px1, double *py1,
	   double *px2, double *py2)
{
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM(text);
	double x, y;
	double x1, x2, y1, y2;
	int cx1, cx2, cy1, cy2;

	adjust_for_anchors(text, &x, &y);

	x1 = x;
	y1 = y;
	x2 = x + text->width;
	y2 = y + text->height;

	gnome_canvas_item_i2w(item, &x1, &y1);
	gnome_canvas_item_i2w(item, &x2, &y2);
	gnome_canvas_w2c(item->canvas, x1, y1, &cx1, &cy1);
	gnome_canvas_w2c(item->canvas, x2, y2, &cx2, &cy2);

	*px1 = cx1;
	*py1 = cy1;
	*px2 = cx2;
	*py2 = cy2;
} /* get_bounds */

static void
gnome_canvas_rich_text_update(GnomeCanvasItem *item, double *affine,
			      ArtSVP *clip_path, int flags)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	double x1, y1, x2, y2;
	GtkTextIter start;

	(* GNOME_CANVAS_ITEM_CLASS(parent_class)->update)(
		item, affine, clip_path, flags);

	get_bounds(text, &x1, &y1, &x2, &y2);

	gtk_text_buffer_get_iter_at_offset(text->buffer, &start, 0);
	gtk_text_layout_validate_yrange(
		text->layout, &start, 0, y2 - y1);

	gnome_canvas_update_bbox(item, x1, y1, x2, y2);
} /* gnome_canvas_rich_text_update */
			       			  
static double
gnome_canvas_rich_text_point(GnomeCanvasItem *item, double x, double y,
			     int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	double ax, ay;
	double x1, x2, y1, y2;
	double dx, dy;

	*actual_item = item;

	/* This is a lame cop-out. Anywhere inside of the bounding box. */

	adjust_for_anchors(text, &ax, &ay);

	x1 = ax;
	y1 = ay;
	x2 = ax + text->width;
	y2 = ay + text->height;

	if ((x > x1) && (y > y1) && (x < x2) && (y < y2))
		return 0.0;

	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	return sqrt(dx * dx + dy * dy);
} /* gnome_canvas_rich_text_point */

static void
gnome_canvas_rich_text_draw(GnomeCanvasItem *item, GdkDrawable *drawable,
			    int x, int y, int width, int height)
{
	GnomeCanvasRichText *text = GNOME_CANVAS_RICH_TEXT(item);
	double i2w[6], w2c[6], i2c[6];
	double ax, ay;
	int x1, y1, x2, y2;
	ArtPoint i1, i2;
	ArtPoint c1, c2;

	GdkRectangle clip;
	GdkGC *gc;

	gnome_canvas_item_i2w_affine(item, i2w);
	gnome_canvas_w2c_affine(item->canvas, w2c);
	art_affine_multiply(i2c, i2w, w2c);

	adjust_for_anchors(text, &ax, &ay);

	i1.x = ax;
	i1.y = ay;
	i2.x = ax + text->width;
	i2.y = ay + text->height;
	art_affine_point(&c1, &i1, i2c);
	art_affine_point(&c2, &i2, i2c);

	x1 = c1.x;
	y1 = c1.y;
	x2 = c2.x;
	y2 = c2.y;

	gtk_text_layout_set_screen_width(text->layout, x2 - x1);

	gtk_text_layout_draw(
		text->layout,
		GTK_WIDGET(item->canvas),
		drawable,
		x - x1, y - y1,
		0, 0, (x2 - x1) - (x - x1), (y2 - y1) - (y - y1));
} /* gnome_canvas_rich_text_draw */

static GtkTextTag *
gnome_canvas_rich_text_add_tag(GnomeCanvasRichText *text, char *tag_name,
			       int start_offset, int end_offset, 
			       const char *first_property_name, ...)
{
	GtkTextTag *tag;
	GtkTextIter start, end;
	va_list var_args;

	g_return_if_fail(text);
	g_return_if_fail(start_offset >= 0);
	g_return_if_fail(end_offset >= 0);

	if (tag_name) {
		GtkTextTagTable *tag_table;

		tag_table = gtk_text_buffer_get_tag_table(get_buffer(text));
		g_return_if_fail(
			gtk_text_tag_table_lookup(tag_table, tag_name) == NULL);
	}

	tag = gtk_text_buffer_create_tag(
		get_buffer(text), tag_name, NULL);

	va_start(var_args, first_property_name);
	g_object_set_valist(G_OBJECT(tag), first_property_name, var_args);
	va_end(var_args);

	gtk_text_buffer_get_iter_at_offset(
		get_buffer(text), &start, start_offset);
	gtk_text_buffer_get_iter_at_offset(
		get_buffer(text), &end, end_offset);
	gtk_text_buffer_apply_tag(get_buffer(text), tag, &start, &end);

	return tag;
} /* gnome_canvas_rich_text_add_tag */

/******** Here is the test code *********/

static gboolean
quit_cb(GtkWidget *widget)
{
	gtk_main_quit();

	return TRUE;
}

static gint
tag_event_handler(GtkTextTag *tag, GnomeCanvasRichText *text, GdkEvent *event,
		  const GtkTextIter *iter, gpointer user_data)
{
	gint char_index;

	char_index = gtk_text_iter_get_offset(iter);

	switch (event->type) {
	case GDK_MOTION_NOTIFY:
		printf ("Motion event at char %d tag `%s'\n",
			char_index, tag->name);
		break;
		
	case GDK_BUTTON_PRESS:
		printf ("Button press at char %d tag `%s'\n",
			char_index, tag->name);
		break;
		
	case GDK_2BUTTON_PRESS:
		printf ("Double click at char %d tag `%s'\n",
			char_index, tag->name);
		break;
        
	case GDK_3BUTTON_PRESS:
		printf ("Triple click at char %d tag `%s'\n",
			char_index, tag->name);
		break;
		
	case GDK_BUTTON_RELEASE:
		printf ("Button release at char %d tag `%s'\n",
			char_index, tag->name);
		break;
		
	case GDK_KEY_PRESS:
		printf ("Key press at char %d tag `%s'\n",
			char_index, tag->name);
		break;
	case GDK_KEY_RELEASE:
		printf ("Key release at char %d tag `%s'\n",
			char_index, tag->name);
		break;
		
	case GDK_DRAG_ENTER:
		printf("Drag enter\n");
		break;
	case GDK_DRAG_LEAVE:
		printf("Drag leave\n");
		break;
	case GDK_DRAG_MOTION:
		printf("Drag motion\n");
		break;
	case GDK_DRAG_STATUS:
		printf("Drag status\n");
		break;
	case GDK_DROP_START:
		printf("Drop start\n");
		break;
	case GDK_DROP_FINISHED:
		printf("Drop finished\n");
		break;

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
	case GDK_PROPERTY_NOTIFY:
	case GDK_SELECTION_CLEAR:
	case GDK_SELECTION_REQUEST:
	case GDK_SELECTION_NOTIFY:
	case GDK_PROXIMITY_IN:
	case GDK_PROXIMITY_OUT:
	default:
		break;
	}
	
	return FALSE;
} /* tag_event_handler */

static gboolean
button_clicked(GtkWidget *canvas, GdkEventButton *event, 
	       GnomeCanvasRichText *text)
{
	GdkColor color;
	static GtkTextTag *tag;

	static int state = 0;

	{
		GnomeCanvasItem *item;
		double worldx, worldy;

		gnome_canvas_window_to_world(
			GNOME_CANVAS(canvas), event->x, event->y,
			&worldx, &worldy);
		item = gnome_canvas_get_item_at(
			GNOME_CANVAS(canvas), worldx, worldy);

		if (item)
			gnome_canvas_item_grab_focus(item);
	}

	if (state == 3)
		return FALSE;

	if (state == 2) {
		state = 3;

		color.blue = color.green = 0;
		color.red = 0xffff;

		g_object_set(G_OBJECT(tag), "foreground_gdk", &color, NULL);
	}

	if (state == 1) {
		state = 2;

		g_object_set(G_OBJECT(text), "height", 60.0, NULL);

#if 0
		gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas), 2.0);
#endif
	}

	if (state == 0) {
		state = 1;


		color.red = color.green = 0;
		color.blue = 0xffff;
		
		tag = gnome_canvas_rich_text_add_tag(
			text, "fg_blue", 4, 15,
			"foreground_gdk", &color,
			"size_points", 20.0,
			"weight", PANGO_WEIGHT_BOLD,
			NULL);
		
		g_signal_connect_data(
			G_OBJECT(tag), "event", G_CALLBACK(tag_event_handler),
			NULL, NULL, FALSE, FALSE);
	}

	return FALSE;
} /* button_clicked */

static void
tag_changed(GnomeCanvasRichText *text, GtkTextTag *tag)
{
	printf("Tag changed to %p\n", tag);
} /* tag_changed */

static void
cut_cb(GtkButton *button, GnomeCanvasRichText *text)
{
	gnome_canvas_rich_text_cut_clipboard(text);
} /* cut_cb */

static void
copy_cb(GtkButton *button, GnomeCanvasRichText *text)
{
	gnome_canvas_rich_text_copy_clipboard(text);
} /* copy_cb */

static void
paste_cb(GtkButton *button, GnomeCanvasRichText *text)
{
	gnome_canvas_rich_text_paste_clipboard(text);
} /* paste_cb */

int 
main(int argc, char *argv[])
{
	GtkWidget *app;
	GtkWidget *canvas;
	GtkWidget *vbox;
	GtkWidget *button;
	GnomeCanvasItem *item;

	gtk_init(&argc, &argv);

	app = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			   GTK_SIGNAL_FUNC(quit_cb), NULL);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(app), vbox);

	canvas = gnome_canvas_new();
	gtk_widget_grab_focus(canvas);
	gtk_box_pack_end(GTK_BOX(vbox), canvas, TRUE, TRUE, 2);

#if 0
	gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas), 2.0);
#endif

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rect_get_type(),
		"x1", -90.0,
		"y1", -50.0,
		"x2", 110.0,
		"y2", 50.0,
		"fill_color", "green",
		"outline_color", "green",
		NULL);

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rich_text_get_type(),
		"x", -90.0,
		"y", -50.0,
		"width", 200.0,
		"height", 100.0,
		"text", 
		"English is so boring because everyone uses it.\n"
		"Here is something exciting:  "
		"وقد بدأ ثلاث من أكثر المؤسسات تقدما في شبكة اكسيون برامجها كمنظمات لا تسعى للربح، ثم تحولت في السنوات الخمس الماضية إلى مؤسسات مالية منظمة، وباتت جزءا من النظام المالي في بلدانها، ولكنها تتخصص في خدمة قطاع المشروعات الصغيرة. وأحد أكثر هذه المؤسسات نجاحا هو »بانكوسول« في بوليفيا.\n"
		"And here is some more plain, boring English.",
		"grow_height", TRUE,
		NULL);

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_ellipse_get_type(),
		"x1", -5.0,
		"y1", -5.0,
		"x2", 5.0,
		"y2", 5.0,
		"fill_color", "white",
		NULL);

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rect_get_type(),
		"x1", 100.0,
		"y1", -30.0,
		"x2", 200.0,
		"y2", 30.0,
		"fill_color", "yellow",
		"outline_color", "yellow",
		NULL);

	item = gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rich_text_get_type(),
		"x", 100.0,
		"y", -30.0,
		"width", 100.0,
		"height", 60.0,
		"text", "The quick brown fox jumped over the lazy dog.\n",
		"cursor_visible", TRUE,
		"cursor_blink", TRUE,
		"grow_height", TRUE, 
		NULL);

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rect_get_type(),
		"x1", 50.0,
		"y1", 70.0,
		"x2", 150.0,
		"y2", 100.0,
		"fill_color", "pink",
		"outline_color", "pink",
		NULL);

	gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rich_text_get_type(),
		"x", 50.0,
		"y", 70.0,
		"width", 100.0,
		"height", 30.0,
		"text", "This is a test.\nI enjoy tests a great deal\nThree lines!",
		"cursor_visible", TRUE,
		"cursor_blink", TRUE,
		NULL);

	gnome_canvas_item_grab_focus(item);

	gtk_signal_connect(
		GTK_OBJECT(canvas), "button_press_event",
		GTK_SIGNAL_FUNC(button_clicked), item);

	g_signal_connect_data(
		G_OBJECT(item), "tag_changed",
		G_CALLBACK(tag_changed), NULL, NULL, FALSE, FALSE);

	button = gtk_button_new_with_label("Cut");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(cut_cb), item);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label("Copy");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(copy_cb), item);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label("Paste");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(paste_cb), item);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 2);

	gtk_widget_show_all(app);

	gtk_main();

	return 0;
} /* main */
