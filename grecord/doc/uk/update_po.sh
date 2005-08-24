#!/bin/bash

xml2po -e -u uk.po \
    ../../../grecord/doc/C/gnome-sound-recorder.xml \
    ../../../gstreamer-properties/help/C/gstreamer-properties.xml \
    ../../../gnome-cd/doc/C/gnome-cd.xml \
    ../../../gst-mixer/doc/C/gnome-volume-control.xml
