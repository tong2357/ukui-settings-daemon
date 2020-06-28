QT -= gui
QT += core widgets x11extras

CONFIG += c++11 console no_keywords link_pkgconfig plugin
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

PKGCONFIG += \
        gtk+-3.0 glib-2.0 x11

SOURCES += \
        usd-locate-pointer.c \
        usd-timeline.c

HEADERS += \
    usd-locate-pointer.h \
    usd-timeline.h


DESTDIR = $$PWD/
locate_pointer.path = /usr/bin/
locate_pointer.files = $$PWD/usd-locate-pointer
INSTALLS += locate_pointer

