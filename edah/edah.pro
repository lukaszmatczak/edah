#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T11:02:49
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = edah
TEMPLATE = app

CONFIG += c++11

gcc:QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

include(../qtsingleapplication/src/qtsingleapplication.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    winframe.cpp \
    aboutdialog.cpp \
    settings.cpp \
    updatedialog.cpp

HEADERS  += mainwindow.h \
    winframe.h \
    aboutdialog.h \
    settings.h \
    updatedialog.h

INCLUDEPATH += ..

LIBS += -L$$PWD/../../build-libedah-Qt5-Debug -ledah

RESOURCES += \
    common.qrc

TRANSLATIONS = lang/lang.pl.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
