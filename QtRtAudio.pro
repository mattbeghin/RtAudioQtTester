#-------------------------------------------------
#
# Project created by QtCreator 2019-05-20T18:10:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtRtAudio
TEMPLATE = app

CONFIG += c++11

DEFINES += _UNICODE

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

    LIBS += -lwinmm -ldsound -lole32 -lgdi32 -luser32 -lAdvapi32
}


