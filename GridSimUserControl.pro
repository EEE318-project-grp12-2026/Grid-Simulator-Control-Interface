QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Suppress warnings about deprecated APIs
DEFINES += QT_DEPRECATED_WARNINGS

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
