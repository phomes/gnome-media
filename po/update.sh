#!/bin/sh

xgettext --default-domain=gnome-media --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gnome-media.po \
   || ( rm -f ./gnome-media.pot \
    && mv gnome-media.po ./gnome-media.pot )
