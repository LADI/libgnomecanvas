INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-I$(top_builddir)/intl				\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(LIBGNOME_CFLAGS)				\
	$(LIBGNOMECANVAS_CFLAGS)			\
        -DGNOMECANVASLIBDIR=\""$(libdir)"\" 		\
        -DGNOMECANVASDATADIR=\""$(datadir)"\" 		\
        -DGNOMECANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DGNOMECANVASBINDIR=\""$(bindir)"\" 		\
        -DGNOMECANVASLOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMECANVASLOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeCANVAS\"			\
	-DVERSION=\"$(VERSION)\"

LDADD =	\
	$(top_builddir)/libgnomecanvas/libgnomecanvas-2.la \
	$(INTLLIBS)

noinst_PROGRAMS = canvas_demo

canvas_demo_SOURCES =		\
	canvas.c		\
	canvas-primitives.c	\
	canvas-arrowhead.c	\
	canvas-features.c	\
	canvas-fifteen.c

