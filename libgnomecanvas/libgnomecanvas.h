/*
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

#ifndef LIBGNOMECANVAS_H
#define LIBGNOMECANVAS_H

#include <libgnomebase/gnome-defs.h>
#include <libgnomebase/gnome-program.h>

#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-line.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include <libgnomecanvas/gnome-canvas-polygon.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-widget.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#include <libgnomecanvas/gnome-canvas-util.h>

G_BEGIN_DECLS

extern GnomeModuleInfo gtk_module_info;
extern GnomeModuleInfo libgnomecanvas_module_info;

void libgnomecanvas_types_init (void);

G_END_DECLS

#endif /* LIBGNOMECANVAS_H */
