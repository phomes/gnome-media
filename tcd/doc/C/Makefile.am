gtcd_helpdir = $(datadir)/gnome/help/tcd/C
gtcd_help_DATA = index.html authors.html bugs.html usage.html topic.dat \
			license.html ln7.html

SGML_FILES =\
	gtcd.sgml

EXTRA_DIST = 		\
	topic.dat 	\
	$(SGML_FILES)

all:

index.html: gtcd/index.html
	-cd $(srcdir) && cp gtcd/index.html .

authors.html: gtcd/authors.html
	-cd $(srcdir) && cp gtcd/authors.html .

usage.html: gtcd/usage.html
	-cd $(srcdir) && cp gtcd/usage.html .

bugs.html: gtcd/bugs.html
	-cd $(srcdir) && cp gtcd/bugs.html .

license.html: gtcd/license.html
	-cd $(srcdir) && cp gtcd/license.html .

ln7.html: gtcd/ln7.html
	-cd $(srcdir) && cp gtcd/ln7.html .

gtcd/index.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

gtcd/usage.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

gtcd/authors.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

gtcd/bugs.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

gtcd/license.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

gtcd/ln7.html: $(SGML_FILES)
	-cd $(srcdir) && db2html gtcd.sgml

dist-hook:
	$(mkinstalldirs) $(distdir)/gtcd
	-cp $(srcdir)/gtcd/*.html gtcd/*.css $(distdir)/gtcd
	-cp $(srcdir)/*.html $(distdir)
	$(mkinstalldirs) $(distdir)/images
	-cp $(srcdir)/images/*.png $(distdir)/images

install-data-local: index.html
	$(mkinstalldirs) $(DESTDIR)$(gtcd_helpdir)/images
	-for file in $(srcdir)/gtcd/*.html $(srcdir)/gtcd/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtcd_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.png ; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtcd_helpdir)/images;\
	done
	$(mkinstalldirs) $(DESTDIR)$(gtcd_helpdir)/stylesheet-images
	-for file in $(srcdir)/gtcd/stylesheet-images/*.gif; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(gtcd_helpdir)/stylesheet-images/$$basefile; \
	done

