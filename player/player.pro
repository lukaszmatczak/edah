#-------------------------------------------------
#
# Project created by QtCreator 2016-05-31T09:29:17
#
#-------------------------------------------------

QT       += core gui widgets network sql
win32: QT += winextras

TARGET = player
TEMPLATE = lib
CONFIG += plugin c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

DESTDIR = ../plugins/player

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/taglib/include C:/Users/lukas/Desktop/bass ../quazip

DEFINES += "QUAZIP_STATIC"

SOURCES += player.cpp \
    bigpanel.cpp \
    settingstab.cpp \
    waveform.cpp \
    smallpanel.cpp \
    playlistmodel.cpp \
    mpv.cpp \
    videowindow.cpp \
    keypad.cpp \
    downloadmanager.cpp \
    ../quazip/quazip/qioapi.cpp \
    ../quazip/quazip/quazip.cpp \
    ../quazip/quazip/quazipfile.cpp \
    ../quazip/quazip/quazipfileinfo.cpp \
    ../quazip/quazip/zip.c \
    ../quazip/quazip/unzip.c \
    windowselector.cpp \
    windowthumbnail.cpp

HEADERS += player.h \
    bigpanel.h \
    settingstab.h \
    waveform.h \
    smallpanel.h \
    playlistmodel.h \
    mpv.h \
    videowindow.h \
    keypad.h \
    downloadmanager.h \
    ../quazip/quazip/qioapi.h \
    ../quazip/quazip/quazip.h \
    ../quazip/quazip/quazipfile.h \
    ../quazip/quazip/quazipfileinfo.h \
    ../quazip/quazip/zip.h \
    ../quazip/quazip/unzip.h \
    windowselector.h \
    windowthumbnail.h

TRANSLATIONS = player-lang/lang.pl.ts player-lang/lang.ru.ts

LIBS += -ltag -lbass

win32 {
    LIBS += -LC:/Users/lukas/Desktop/bass -LC:/Users/lukas/Desktop/zlib/lib -ledah -lole32 -lzdll -luser32 -lgdi32 -ldwmapi -lshell32
}

CONFIG(debug) {
    win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_9_2_MSVC2015_32bit-Debug/debug -LC:/Users/lukas/Desktop/taglib/debug/lib
}

CONFIG(release) {
    win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_9_2_MSVC2015_32bit-Release/release -LC:/Users/lukas/Desktop/taglib/lib
}

RESOURCES += \
    common.qrc

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
