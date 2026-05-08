QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Suppress warnings about deprecated APIs
DEFINES += QT_DEPRECATED_WARNINGS

# Qt 6.9 added a version-tag symbol (_qt_version_tag_6_9) to MOC-generated
# files to detect Qt version mismatches at link time. With certain MSVC
# toolchain setups the linker fails to resolve it from Qt6Core.lib even
# when the correct Qt is installed (LNK2001: unresolved external symbol
# __imp__qt_version_tag_6_9). Disabling the tag is safe for in-house builds.
DEFINES += QT_NO_VERSION_TAGGING

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    serialhandler.cpp \
    DarkeumStyle.cpp

HEADERS += \
    mainwindow.h \
    serialhandler.h \
    DarkeumStyle.h

FORMS += \
    mainwindow.ui

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
