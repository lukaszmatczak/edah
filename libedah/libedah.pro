#-------------------------------------------------
#
# Project created by QtCreator 2016-06-08T11:38:38
#
#-------------------------------------------------

QT       += gui widgets sql

TARGET = edah
TEMPLATE = lib

CONFIG += c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DEFINES += LIBEDAH_LIBRARY

SOURCES += logger.cpp \
    utils.cpp \
    mypushbutton.cpp \
    peakmeter.cpp \
    multilangstring.cpp

HEADERS += iplugin.h \
        logger.h \
        utils.h \
    mypushbutton.h \
    libedah.h \
    peakmeter.h \
    multilangstring.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
