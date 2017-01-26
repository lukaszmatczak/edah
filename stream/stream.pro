#-------------------------------------------------
#
# Project created by QtCreator 2016-12-09T14:14:55
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = stream
TEMPLATE = lib
CONFIG += plugin c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/stream

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/bass

SOURCES += stream.cpp \
    settingstab.cpp \
    bigpanel.cpp \
    smallpanel.cpp

HEADERS += stream.h \
    settingstab.h \
    bigpanel.h \
    smallpanel.h

LIBS += -L. -lbass
win32: LIBS += -LC:/Users/lukas/Desktop/bass -ledah

CONFIG(debug) {
    win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Debug/debug
}

RESOURCES += \
    common.qrc

TRANSLATIONS = stream-lang/lang.pl.ts stream-lang/lang.ru.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
