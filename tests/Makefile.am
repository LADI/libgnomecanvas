check_PROGRAMS = \
	test-libglade-canvas

check_DATA = \
	test-libglade-canvas.glade2

TESTS = $(check_PROGRAMS)

TESTS_ENVIRONMENT = \
	LIBGLADE_MODULE_PATH="$(top_builddir)/glade/.libs:$LIBGLADE_MODULE_PATH"

INCLUDES = \
	-I$(top_srcdir)		\
	$(CANVAS_GLADE_CFLAGS)

test-libglade-canvas.glade2: test-libglade-canvas.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) \
		$(srcdir)/test-libglade-canvas.glade \
		> test-libglade-canvas.glade2.tmp \
	&& mv test-libglade-canvas.glade2.tmp \
		test-libglade-canvas.glade2

LDADD = $(CANVAS_GLADE_LIBS)

LDFLAGS = -export-dynamic

test_libglade_canvas_SOURCE = test-libglade-canvas.c

EXTRA_DIST = test-libglade-canvas.glade

distclean-local:
	rm -f test-libglade-canvas.glade2
