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

libgnomecanvasincludedir = $(includedir)/gnome/2/libgnomecanvas

libgnomecanvas_headers =			\
	gnome-canvas-line.h			\
	gnome-canvas-pixbuf.h			\
	gnome-canvas-polygon.h			\
	gnome-canvas-rect-ellipse.h		\
	gnome-canvas-text.h			\
	gnome-canvas-util.h			\
	gnome-canvas-widget.h			\
	gnome-canvas.h				\
	libgnomecanvas.h

libgnomecanvasinclude_HEADERS =			\
	$(libgnomecanvas_headers)

libgnomecanvas_2_la_SOURCES =			\
	$(libgnomecanvasinclude_HEADERS)	\
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

BUILT_SOURCES = \
	libgnomecanvastypebuiltins.h		\
	libgnomecanvastypebuiltins_evals.c	\
	libgnomecanvastypebuiltins_ids.c	\
	libgnomecanvastypebuiltins_vars.c

# generate libgnomecanvas.defs file from libgnomecanvas-boxed.defs and *.h
libgnomecanvas.defs: $(GNOME_MAKEENUMS) libgnomecanvas-boxed.defs $(libgnomecanvas_headers)
	cd $(srcdir) \
	&& $(PERL) $(GNOME_MAKEENUMS) defs $(libgnomecanvas_headers) > gd.tmp \
	&& cat libgnomecanvas-boxed.defs >> gd.tmp \
	&& mv gd.tmp libgnomecanvas.defs

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
libgnomecanvastypebuiltins.h: libgnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnomecanvas.defs macros > gtb.tmp \
	&& mv gtb.tmp libgnomecanvastypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
libgnomecanvastypebuiltins_vars.c: libgnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnomecanvas.defs variables > gtbv.tmp \
	&& mv gtbv.tmp libgnomecanvastypebuiltins_vars.c

# generate type entries for type-id registration
libgnomecanvastypebuiltins_ids.c: libgnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnomecanvas.defs entries > gtbi.tmp \
	&& mv gtbi.tmp libgnomecanvastypebuiltins_ids.c

# generate enum value arrays
libgnomecanvastypebuiltins_evals.c: $(GNOME_MAKEENUMS) libgnomecanvas.defs
	cd $(srcdir) \
	&& $(PERL) $(GNOME_MAKEENUMS) arrays $(libgnomecanvas_headers) > gtbe.tmp \
	&& mv gtbe.tmp libgnomecanvastypebuiltins_evals.c

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = libgnomecanvas-2.0.pc.in \
	libgnomecanvas.defs libgnomecanvas-boxed.defs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomecanvas-2.0.pc
