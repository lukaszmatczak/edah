#-------------------------------------------------
#
# Project created by QtCreator 2016-06-08T11:38:38
#
#-------------------------------------------------

QT       += gui widgets network

TARGET = edah
TEMPLATE = lib

CONFIG += c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DEFINES += LIBEDAH_LIBRARY

SOURCES += logger.cpp \
    utils.cpp \
    mypushbutton.cpp \
    peakmeter.cpp \
    multilangstring.cpp \
    bluropacityeffect.cpp \
    popup.cpp \
    thumbnailwidget.cpp \
    flickcharm.cpp

HEADERS += iplugin.h \
        logger.h \
        utils.h \
    mypushbutton.h \
    libedah.h \
    peakmeter.h \
    multilangstring.h \
    bluropacityeffect.h \
    popup.h \
    thumbnailwidget.h \
    flickcharm.h

win32 {
    SOURCES += updater.cpp
    HEADERS += updater.h

    LIBS += -L../openssl/lib -ladvapi32 -lshell32 -llibeay32MD  -luser32 -lgdi32 -ldwmapi

    INCLUDEPATH += ../openssl/include
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}
