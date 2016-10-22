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

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/taglib/include C:/Users/lukas/Desktop/bass

SOURCES += player.cpp \
    bigpanel.cpp \
    settingstab.cpp \
    waveform.cpp \
    smallpanel.cpp \
    playlistmodel.cpp \
    mpv.cpp \
    videowindow.cpp \
    keypad.cpp

HEADERS += player.h \
    bigpanel.h \
    settingstab.h \
    waveform.h \
    smallpanel.h \
    playlistmodel.h \
    mpv.h \
    videowindow.h \
    keypad.h

TRANSLATIONS = player-lang/lang.pl.ts

LIBS += -ltag -lbass
win32: LIBS += -LC:/Users/lukas/Desktop/bass -ledah -lole32

CONFIG(debug) {
    #win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Debug/debug -LC:/Users/lukas/Desktop/taglib/debug/lib
}

CONFIG(release) {
    win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Release/release -LC:/Users/lukas/Desktop/taglib/lib
}

RESOURCES += \
    common.qrc

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
