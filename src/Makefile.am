bin_PROGRAMS = tihos

tihos_SOURCES = util.c tihos.c
tihos_LDADD = $(DEPS_LIBS)

AM_CPPFLAGS = $(DEPS_CFLAGS) \
              -DPIXMAPS_DIR=\""$(datadir)/pixmaps"\" \
              -DLIB_DIR=\""$(libdir)"\" \
              -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

