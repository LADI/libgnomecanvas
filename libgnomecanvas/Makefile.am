INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DBONOBO_UIDIR=\""$(datadir)/gnome/ui/"\"	\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-DG_LOG_DOMAIN=\"Bonobo\"			\
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(builddir)					\
        -I$(includedir)                                 \
	$(WARN_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(LIBGNOME_CFLAGS)				\
	$(LIBART_CFLAGS)				\
	$(PANGO_CFLAGS)

lib_LTLIBRARIES = libgnomecanvas-2.la

libgnomecanvas_2_la_LIBADD = \
	$(LIBGNOME_LIBS) $(LIBART_LIBS) $(PANGO_LIBS)

libgnomecanvasincludedir = $(includedir)/gnome/2/libgnomecanvas

libgnomecanvasinclude_HEADERS =			\
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
	gnome-canvas-line.c			\
	gnome-canvas-pixbuf.c			\
	gnome-canvas-polygon.c			\
	gnome-canvas-rect-ellipse.c		\
	gnome-canvas-text.c			\
	gnome-canvas-util.c			\
	gnome-canvas-widget.c			\
	gnome-canvas.c				\
	gnomecanvastypes.c

BUILT_SOURCES = \
	gnomecanvastypebuiltins.h		\
	gnomecanvastypebuiltins_evals.c		\
	gnomecanvastypebuiltins_ids.c		\
	gnomecanvastypebuiltins_vars.c

# generate gnomecanvas.defs file from gnomecanvas-boxed.defs and *.h
gnomecanvas.defs: @REBUILD@ $(GNOME_MAKEENUMS) gnomecanvas-boxed.defs $(gnomecanvas_headers)
	cd $(srcdir) \
	&& $(PERL) $(GNOME_MAKEENUMS) defs $(gnomecanvas_headers) > gd.tmp \
	&& cat gnomecanvas-boxed.defs >> gd.tmp \
	&& mv gd.tmp gnomecanvas.defs

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
gnomecanvastypebuiltins.h: @REBUILD@ gnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnomecanvas.defs macros > gtb.tmp \
	&& mv gtb.tmp gnomecanvastypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
gnomecanvastypebuiltins_vars.c: @REBUILD@ gnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnomecanvas.defs variables > gtbv.tmp \
	&& mv gtbv.tmp gnomecanvastypebuiltins_vars.c

# generate type entries for type-id registration
gnomecanvastypebuiltins_ids.c: @REBUILD@ gnomecanvas.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnomecanvas.defs entries > gtbi.tmp \
	&& mv gtbi.tmp gnomecanvastypebuiltins_ids.c

# generate enum value arrays
gnomecanvastypebuiltins_evals.c: @REBUILD@ $(GNOME_MAKEENUMS) gnomecanvas.defs
	cd $(srcdir) \
	&& $(PERL) $(GNOME_MAKEENUMS) arrays $(gnomecanvas_headers) > gtbe.tmp \
	&& mv gtbe.tmp gnomecanvastypebuiltins_evals.c

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = libgnomecanvas-2.0.pc.in $(GNOME_MAKEENUMS) $(GNOME_MAKETYPES) \
	gnomecanvas-boxed.defs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomecanvas-2.0.pc
