INCLUDES = \
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(builddir)					\
        -I$(includedir)                                 \
	$(WARN_CFLAGS) -Wno-unused			\
	$(LIBGNOMECANVAS_CFLAGS)			\
        -DGNOMECANVASLIBDIR=\""$(libdir)"\" 		\
        -DGNOMECANVASDATADIR=\""$(datadir)"\" 		\
        -DGNOMECANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DGNOMECANVASBINDIR=\""$(bindir)"\" 		\
        -DGNOMECANVASLOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMECANVASLOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeCanvas\"			\
	-DVERSION=\"$(VERSION)\"

lib_LTLIBRARIES = libgnomecanvas-2.la

libgnomecanvas_2_la_LIBADD = \
	$(LIBGNOMECANVAS_LIBS)

libincludedir = $(includedir)/gnome/2

libgnomecanvasincludedir = $(includedir)/gnome/2/libgnomecanvas

libinclude_HEADERS = \
	libgnomecanvas.h

libgnomecanvasinclude_HEADERS =			\
	gnome-canvas-shape.h			\
	gnome-canvas-bpath.h			\
	gnome-canvas-clipgroup.h		\
	gnome-canvas-line.h			\
	gnome-canvas-pixbuf.h			\
	gnome-canvas-polygon.h			\
	gnome-canvas-rect-ellipse.h		\
	gnome-canvas-text.h			\
	gnome-canvas-rich-text.h		\
	gnome-canvas-util.h			\
	gnome-canvas-widget.h			\
	gnome-canvas-path-def.h			\
	gnome-canvas.h

libgnomecanvas_2_la_SOURCES =			\
	$(libgnomecanvasinclude_HEADERS)	\
	gnome-canvas-shape.c			\
	gnome-canvas-shape-private.h		\
	gnome-canvas-bpath.c			\
	gnome-canvas-path-def.c			\
	gnome-canvas-clipgroup.c		\
	gnome-canvas-i18n.h			\
	gnome-canvas-line.c			\
	gnome-canvas-pixbuf.c			\
	gnome-canvas-polygon.c			\
	gnome-canvas-rect-ellipse.c		\
	gnome-canvas-text.c			\
	gnome-canvas-rich-text.c		\
	gnome-canvas-util.c			\
	gnome-canvas-widget.c			\
	gnome-canvas.c				\
	libgnomecanvastypes.c

marshal_sources =				\
	gnome-canvas-marshal.c			\
	gnome-canvas-marshal.h

BUILT_SOURCES =					\
	$(marshal_sources)

$(srcdir)/gnome-canvas-marshal.h: @REBUILD@ Makefile.am gnome-canvas-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gnome_canvas_marshal gnome-canvas-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh gnome-canvas-marshal.h || cp xgen-gmh gnome-canvas-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

$(srcdir)/gnome-canvas-marshal.c: @REBUILD@ Makefile.am gnome-canvas-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gnome_canvas_marshal gnome-canvas-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc gnome-canvas-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

EXTRA_DIST = \
	gnome-canvas-marshal.list	\
	gnome-canvas-marshal.c		\
	gnome-canvas-marshal.h		\
	libgnomecanvas-2.0.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomecanvas-2.0.pc
