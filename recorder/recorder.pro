#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T11:59:47
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = recorder
TEMPLATE = lib
CONFIG += plugin c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/recorder

INCLUDEPATH += ..

SOURCES += recorder.cpp \
    settingstab.cpp \
    bigpanel.cpp \
    smallpanel.cpp

HEADERS += recorder.h \
    settingstab.h \
    bigpanel.h \
    smallpanel.h

LIBS += -L. -lbass -lbassenc
win32: LIBS += -ledah

RESOURCES += \
    common.qrc
