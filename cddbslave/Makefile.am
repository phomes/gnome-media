INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR) \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = cddbslave

cddbslave_SOURCES = 	socket.c main.c socket.h cddb.h

cddbslave_LDADD =    	$(GNOME_LIBDIR) \
                	$(GNOMEUI_LIBS) \
                	$(INTLLIBS)

CLEANFILES=*~