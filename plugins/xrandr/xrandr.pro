QT += gui
QT += core xml widgets x11extras dbus

TEMPLATE = lib
DEFINES += XRANDR_LIBRARY

CONFIG += c++11 no_keywords link_pkgconfig plugin xrandr
CONFIG += app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common/          \
        -I ukui-settings-daemon/        \
        -I /usr/include/mate-desktop-2.0/
LIBS += \
        /usr/lib/x86_64-linux-gnu/libmate-desktop-2.so


PKGCONFIG += \
            xrandr x11 gtk+-3.0 glib-2.0

SOURCES += \
    xrandr-manager.cpp \
    xrandr-plugin.cpp

HEADERS += \
    xrandr-manager.h \
    xrandr_global.h \
    xrandr-plugin.h

DESTDIR = $$PWD/
xrandr_lib.path  = /usr/local/lib/ukui-settings-daemon/
xrandr_lib.files = $$PWD/libxrandr.so

INSTALLS += xrandr_lib

