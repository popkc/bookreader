#-------------------------------------------------
#
# Project created by QtCreator 2018-05-24T18:16:32
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
equals(QT_MAJOR_VERSION, 6): QT += core5compat

!win32 {
DEFINES += BOOKREADER_USE_QTSPEECH
}

contains(DEFINES, BOOKREADER_USE_QTSPEECH) {
QT+= texttospeech
} else {
SOURCES +=core/texttospeech.cpp
HEADERS +=core/texttospeech.h
}

win32: RC_ICONS += icon.ico

TARGET = bookreader
TEMPLATE = app
VERSION=1.1.3
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

PRECOMPILED_HEADER=pch.h
PRECOMPILED_DIR=pch
CONFIG += precompile_header c++11

include(qhotkey.pri)


SOURCES += \
    dialog/dialogabout.cpp \
        main.cpp \
        mainwindow.cpp \
    core/widgetoutput.cpp \
    core/fileinfo.cpp \
    core/widgetfull.cpp \
    core/widgetoneline.cpp \
    dialog/dialogconfig.cpp \
    dialog/dialogsearch.cpp \
    dialog/dialogindex.cpp \
    dialog/dialogindexadvance.cpp \
    dialog/formrollrate.cpp

HEADERS += \
    dialog/dialogabout.h \
        mainwindow.h \
    pch.h \
    core/widgetoutput.h \
    core/fileinfo.h \
    core/widgetfull.h \
    core/widgetoneline.h \
    dialog/dialogconfig.h \
    dialog/dialogsearch.h \
    dialog/dialogindex.h \
    dialog/dialogindexadvance.h \
    dialog/formrollrate.h

FORMS += \
    dialog/dialogabout.ui \
        mainwindow.ui \
    dialog/dialogconfig.ui \
    dialog/dialogsearch.ui \
    dialog/dialogindex.ui \
    dialog/dialogindexadvance.ui \
    dialog/formrollrate.ui

DISTFILES +=

RESOURCES += \
    res.qrc
