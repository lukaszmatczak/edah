#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

DESTDIR = ../plugins/player

INCLUDEPATH += ..

SOURCES += player.cpp

HEADERS += player.h

DISTFILES += player.json
