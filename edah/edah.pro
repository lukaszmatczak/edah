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

CONFIG(debug) {
    LIBS += -L$$PWD/../../build-libedah-Qt_5_5_0_msvc2010-Debug/debug
}

CONFIG(release) {
    LIBS += -L$$PWD/../../build-libedah-Qt_5_5_0_msvc2010-Release/release
}

LIBS += -L$$PWD/../libedah -ledah

RC_FILE = res.rc

RESOURCES += \
    common.qrc

TRANSLATIONS = lang/lang.pl.ts

CONFIG(debug) {
MY_DESTDIR_TARGET = "$$OUT_PWD/debug"
}
CONFIG(release) {
MY_DESTDIR_TARGET = "$$OUT_PWD/release"
}

win32:CONFIG(release, debug|release) {
    WINSDK_DIR = C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A
    WIN_PWD = $$replace(PWD, /, \\)
    DESTDIR_TARGET_WIN = $$replace(MY_DESTDIR_TARGET, /, \\)
    QMAKE_POST_LINK += "$$WINSDK_DIR/bin/mt.exe -manifest $$quote($$WIN_PWD\\$$basename(TARGET).exe.manifest) -outputresource:$$quote($$DESTDIR_TARGET_WIN\\$$basename(TARGET).exe;1)"
}

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN}
lrelease.CONFIG       += no_link target_predeps
