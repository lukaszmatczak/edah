#-------------------------------------------------
#
# Project created by QtCreator 2016-06-08T11:38:38
#
#-------------------------------------------------

QT       -= gui

TARGET = edah
TEMPLATE = lib

DEFINES += LIBEDAH_LIBRARY

SOURCES += logger.cpp \
    utils.cpp

HEADERS += iplugin.h\
        logger.h \
    utils.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
