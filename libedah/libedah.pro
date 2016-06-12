#-------------------------------------------------
#
# Project created by QtCreator 2016-06-08T11:38:38
#
#-------------------------------------------------

QT       += gui widgets sql

TARGET = edah
TEMPLATE = lib

CONFIG += c++11

DEFINES += LIBEDAH_LIBRARY

SOURCES += logger.cpp \
    utils.cpp \
    mypushbutton.cpp \
    database.cpp

HEADERS += iplugin.h \
        logger.h \
        utils.h \
        database.h \
    mypushbutton.h \
    libedah.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
