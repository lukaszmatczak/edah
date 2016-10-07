#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets
win32: QT += winextras

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/player

INCLUDEPATH += .. C:/Users/lukasz/Desktop/taglib/include

SOURCES += player.cpp \
    bigpanel.cpp \
    settingstab.cpp \
    waveform.cpp \
    smallpanel.cpp \
    playlistmodel.cpp \
    mpv.cpp \
    videowindow.cpp

HEADERS += player.h \
    bigpanel.h \
    settingstab.h \
    waveform.h \
    smallpanel.h \
    playlistmodel.h \
    mpv.h \
    videowindow.h

DISTFILES += player.json

TRANSLATIONS = player-lang/lang.pl.ts

LIBS += -LC:/Users/lukasz/Desktop/taglib/lib -ltag -lbass
win32: LIBS += -ledah -lole32

RESOURCES += \
    common.qrc

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
