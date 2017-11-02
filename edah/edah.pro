#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T11:02:49
#
#-------------------------------------------------

QT       += core gui widgets network svg

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
    updatedialog.cpp \
    splashscreen.cpp \
    closingpopup.cpp

HEADERS  += mainwindow.h \
    winframe.h \
    aboutdialog.h \
    settings.h \
    updatedialog.h \
    splashscreen.h \
    closingpopup.h

INCLUDEPATH += ..

CONFIG(debug) {
    LIBS += -L$$PWD/../../build-libedah-Desktop_Qt_5_9_2_MSVC2015_32bit-Debug/debug
}

CONFIG(release) {
    LIBS += -L$$PWD/../../build-libedah-Desktop_Qt_5_9_2_MSVC2015_32bit-Release/release
}

LIBS += -L$$PWD/../libedah -ledah

win32 {
    LIBS += -ladvapi32 -luser32
}

RC_FILE = res.rc

RESOURCES += \
    common.qrc

TRANSLATIONS = lang/lang.pl.ts lang/lang.ru.ts

CONFIG(debug) {
MY_DESTDIR_TARGET = "$$OUT_PWD/debug"
}
CONFIG(release, release|debug) {
MY_DESTDIR_TARGET = "$$OUT_PWD/release"
}

win32 {
    WINSDK_DIR = "C:/Program Files (x86)/Windows Kits/8.1/bin/x86"
    WIN_PWD = $$replace(PWD, /, \\)
    DESTDIR_TARGET_WIN = $$replace(MY_DESTDIR_TARGET, /, \\)
    QMAKE_POST_LINK += "$$WINSDK_DIR/mt.exe -manifest $$quote($$WIN_PWD\\$$basename(TARGET).exe.manifest) -outputresource:$$quote($$DESTDIR_TARGET_WIN\\$$basename(TARGET).exe;1)"
}

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
