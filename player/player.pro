#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/player

INCLUDEPATH += ..

SOURCES += player.cpp \
    bigpanel.cpp \
    settingstab.cpp \
    waveform.cpp \
    smallpanel.cpp

HEADERS += player.h \
    bigpanel.h \
    settingstab.h \
    waveform.h \
    smallpanel.h

DISTFILES += player.json

LIBS += -ltag -lbass
win32: LIBS += -ledah

RESOURCES += \
    common.qrc
