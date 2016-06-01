#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T11:02:49
#
#-------------------------------------------------

QT       += core gui widgets network sql

TARGET = edah
TEMPLATE = app

CONFIG += c++11

include(../qtsingleapplication/src/qtsingleapplication.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    mypushbutton.cpp \
    logger.cpp \
    winframe.cpp \
    aboutdialog.cpp \
    settings.cpp \
    database.cpp

HEADERS  += mainwindow.h \
    mypushbutton.h \
    osutils.h \
    logger.h \
    winframe.h \
    aboutdialog.h \
    settings.h \
    database.h \
    iplugin.h

linux: SOURCES += linuxutils.cpp

linux: HEADERS += linuxutils.h

win32: SOURCES += windowsutils.cpp

win32: HEADERS += windowsutils.h

RESOURCES += \
    common.qrc

TRANSLATIONS = lang/lang.pl.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
