#!/bin/sh
aclocal -I . --verbose
automake --add-missing
autoconf