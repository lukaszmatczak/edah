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

win32:INCLUDEPATH += .. C:/Users/lukas/Desktop/bass

SOURCES += recorder.cpp \
    settingstab.cpp \
    bigpanel.cpp \
    smallpanel.cpp

HEADERS += recorder.h \
    settingstab.h \
    bigpanel.h \
    smallpanel.h

LIBS += -L. -lbass -lbassenc
win32: LIBS += -LC:/Users/lukas/Desktop/bass -ledah

CONFIG(debug) {
    win32: LIBS += -LC:/Users/lukas/Desktop/edah/build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Debug/debug
}

RESOURCES += \
    common.qrc

TRANSLATIONS = recorder-lang/lang.pl.ts recorder-lang/lang.ru.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
