QT       -= core gui widgets

TARGET = voip
TEMPLATE = app
CONFIG += c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/stream

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/opal/opal/include C:/Users/lukas/Desktop/opal/ptlib/include

SOURCES += $$PWD/voip.cpp \
    voip.cpp

HEADERS += $$PWD/voip.h \
    voip.h

LIBS += -L. -lopal -lptlib
win32: LIBS += -LC:/Users/lukas/Desktop/opal/ptlib/lib -LC:/Users/lukas/Desktop/opal/opal/lib
