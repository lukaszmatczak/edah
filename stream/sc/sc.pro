QT       -= core gui widgets

TARGET = sc
TEMPLATE = app
CONFIG += c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/stream

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/bass

SOURCES += $$PWD/sc.cpp

HEADERS += $$PWD/sc.h

LIBS += -L. -lbass -lbassenc
win32: LIBS += -LC:/Users/lukas/Desktop/bass
