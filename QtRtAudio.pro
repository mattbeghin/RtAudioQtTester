#-------------------------------------------------
#
# Project created by QtCreator 2019-05-20T18:10:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtRtAudio
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    devicedialog.cpp

HEADERS += \
        mainwindow.h \
    devicedialog.h \
    definitions.h

FORMS += \
        mainwindow.ui \
    devicedialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += ./RtAudio ./RtAudio/include
SOURCES += ./RtAudio/RtAudio.cpp
HEADERS += ./RtAudio/RtAudio.h

mac {
    #RtMidi & RtAudio
    DEFINES += __MACOSX_CORE__

    LIBS += -framework CoreFoundation -framework CoreAudio
}

windows {
    #RtAudio
    DEFINES += __WINDOWS_DS__
    DEFINES += __WINDOWS_ASIO__
    DEFINES += __WINDOWS_WASAPI__

    SOURCES += \
        ./RtAudio/include/asio.cpp \
        ./RtAudio/include/asiodrivers.cpp \
        ./RtAudio/include/asiolist.cpp \
        ./RtAudio/include/iasiothiscallresolver.cpp
}


