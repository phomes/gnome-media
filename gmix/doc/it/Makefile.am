gmix_helpdir = $(datadir)/gnome/help/gmix/it
gmix_help_DATA = index.html authors.html bugs.html gmix-use.html topic.dat \
			license.html ln7.html gmix-prefs.html

SGML_FILES =\
	gmix.sgml

EXTRA_DIST = 		\
	topic.dat 	\
	$(SGML_FILES)

all:

index.html: gmix/index.html
	-cd $(srcdir) && cp gmix/index.html .

authors.html: gmix/authors.html
	-cd $(srcdir) && cp gmix/authors.html .

gmix-prefs.html: gmix/gmix-prefs.html
	-cd $(srcdir) && cp gmix/gmix-prefs.html .

gmix-use.html: gmix/gmix-use.html
	-cd $(srcdir) && cp gmix/gmix-use.html .

bugs.html: gmix/bugs.html
	-cd $(srcdir) && cp gmix/bugs.html .

license.html: gmix/license.html
	-cd $(srcdir) && cp gmix/license.html .

ln7.html: gmix/ln7.html
	-cd $(srcdir) && cp gmix/ln7.html .

gmix/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

gmix/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

gmix/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

gmix/bugs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

gmix/license.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

gmix/ln7.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gmix.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gmix
	-cp $(srcdir)/gmix/*.html gmix/*.css $(distdir)/gmix
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/figs
	-cp $(srcdir)/figs/*.png $(distdir)/figs

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gmix_helpdir)/figs
	-for file in $(srcdir)/gmix/*.html $(srcdir)/gmix/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gmix_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/figs/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gmix_helpdir)/figs;\
	done
	$(mkinstalldirs) $(DESTDIR)$(gmix_helpdir)/stylesheet-images
	-for file in $(srcdir)/gmix/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gmix_helpdir)/stylesheet-images/$$basefile; \
	done

