#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gnome-media"
REQUIRED_AUTOMAKE_VERSION=1.9
REQUIRED_INTLTOOL_VERSION=0.35
REQUIRED_YELP_TOOLS_VERSION=3.1.1

(test -f $srcdir/configure.ac \
  && test -d $srcdir/grecord) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common 2.4.0 or higher"
    exit 1
}

. gnome-autogen.sh "$@"
