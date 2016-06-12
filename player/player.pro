#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets sql multimedia

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

DESTDIR = ../plugins/player

INCLUDEPATH += ..

SOURCES += player.cpp \
    bigpanel.cpp \
    settingstab.cpp

HEADERS += player.h \
    bigpanel.h \
    settingstab.h

DISTFILES += player.json

LIBS += -ltag -ledah
