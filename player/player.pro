#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

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

TRANSLATIONS = player-lang/lang.pl.ts

LIBS += -L. -ltag -lbass
win32: LIBS += -ledah

RESOURCES += \
    common.qrc

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
