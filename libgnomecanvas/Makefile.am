INCLUDES = \
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(builddir)					\
        -I$(includedir)                                 \
	$(WARN_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(LIBART_CFLAGS)				\
	$(PANGO_CFLAGS)					\
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
	$(GTK_LIBS) $(LIBART_LIBS) $(PANGO_LIBS)

libincludedir = $(includedir)/gnome/2

libgnomecanvasincludedir = $(includedir)/gnome/2/libgnomecanvas

libinclude_HEADERS = \
	libgnomecanvas.h

libgnomecanvasinclude_HEADERS =			\
	gnome-canvas-bpath.h			\
	gnome-canvas-line.h			\
	gnome-canvas-pixbuf.h			\
	gnome-canvas-polygon.h			\
	gnome-canvas-rect-ellipse.h		\
	gnome-canvas-text.h			\
	gnome-canvas-util.h			\
	gnome-canvas-widget.h			\
	gnome-canvas.h

libgnomecanvas_2_la_SOURCES =			\
	$(libgnomecanvasinclude_HEADERS)	\
	gnome-canvas-bpath.c			\
	gp-path.c				\
	gnome-canvas-i18n.h			\
	gnome-canvas-line.c			\
	gnome-canvas-pixbuf.c			\
	gnome-canvas-polygon.c			\
	gnome-canvas-rect-ellipse.c		\
	gnome-canvas-text.c			\
	gnome-canvas-util.c			\
	gnome-canvas-widget.c			\
	gnome-canvas.c				\
	libgnomecanvastypes.c

EXTRA_DIST = libgnomecanvas-2.0.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomecanvas-2.0.pc
