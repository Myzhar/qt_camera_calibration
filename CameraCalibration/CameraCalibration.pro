#-------------------------------------------------
#
# Project created by QtCreator 2017-11-26T12:12:13
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

TARGET = CameraCalibration
TEMPLATE = app

INCLUDEPATH += include

include(libs/opencv.pri)
include(libs/gst_sink_opencv/gst_sink_opencv.pri)
include(libs/qt_opencv_scene/qt_opencv_scene.pri)


SOURCES += \
    src/main.cpp\
    src/mainwindow.cpp \
    src/qchessboardelab.cpp \
    src/cameraman.cpp \
    src/qcameracalibrate.cpp \
    src/cameraundistort.cpp

HEADERS  += \
    include/mainwindow.h \
    include/qchessboardelab.h \
    include/cameraman.h \
    include/qcameracalibrate.h \
    include/cameraundistort.h

FORMS    += \
            forms/mainwindow.ui

RESOURCES += \
    resources.qrc
