#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T11:59:47
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = recorder
TEMPLATE = lib
CONFIG += plugin c++11

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/recorder

INCLUDEPATH += ..

SOURCES += recorder.cpp

HEADERS += recorder.h

DISTFILES += recorder.json
