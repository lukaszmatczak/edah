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

TRANSLATIONS = recorder-lang/lang.pl.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
