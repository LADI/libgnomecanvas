## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"GnomeCanvas\"			\
	$(CANVAS_GLADE_CFLAGS)

glademoduledir = $(GLADE_MODULE_DIR)
glademodule_LTLIBRARIES = libcanvas.la

libcanvas_la_LIBADD = \
	$(top_builddir)/libgnomecanvas/libgnomecanvas-2.la	\
	$(CANVAS_GLADE_LIBS)

libcanvas_la_LDFLAGS = \
	-export-dynamic -module -avoid-version

libcanvas_la_SOURCES = \
	glade-canvas.c
