#-------------------------------------------------
#
# Project created by QtCreator 2016-08-25T13:27:11
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = updater
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

QMAKE_LFLAGS += "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""

INCLUDEPATH += ..

CONFIG(debug) {
    LIBS +=  -L$$PWD/../../build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Debug/debug
}

CONFIG(release) {
    LIBS +=  -L$$PWD/../../build-libedah-Desktop_Qt_5_7_0_MSVC2015_32bit-Release/release
}

LIBS += -ledah -ladvapi32 -luser32

RC_FILE = res.rc

TRANSLATIONS = lang/lang.pl.ts lang/lang.ru.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps

RESOURCES += \
    common.qrc
