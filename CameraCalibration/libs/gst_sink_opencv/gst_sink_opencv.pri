message("Added gst_sink_opencv")

PATH = $$PWD
INC = $$PATH/include
SRC = $$PATH/src

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$INC

HEADERS += \
            $$INC/gst_sink_opencv.hpp \
            $$INC/camerathread.h

SOURCES += \
            $$SRC/gst_sink_opencv.cpp \
            $$SRC/camerathread.cpp

#-------------------------------------------------
win32{
    message("Compiling for Win32")
    GSTREAMER_PATH = C:\gstreamer\1.0\x86

    LIBS += \
    -lintl \
    -lws2_32 \
    -lole32 \
    -lwinmm \
    -lshlwapi \
    -lffi
}

linux{
    message("Compiling for linux-g++")
    GSTREAMER_PATH = /usr

    #########################################################################
    # Jetson TX1/TX2
    #message("Jetson TX1/TX2")
    #INCLUDEPATH += \
    #    $$GSTREAMER_PATH/lib/aarch64-linux-gnu/glib-2.0/include \
    #    $$GSTREAMER_PATH/lib/aarch64-linux-gnu/gstreamer-1.0/include
    #########################################################################

    #########################################################################
    # ARM 32bit
    #message("ARM 32bit (Rpi3)")
    #INCLUDEPATH += $$GSTREAMER_PATH/lib/arm-linux-gnueabihf/glib-2.0/include
    #########################################################################

    #########################################################################
    # Linux Desktop 64 bit
    message("Linux Desktop 64 bit")
    INCLUDEPATH += \
        $$GSTREAMER_PATH/lib/x86_64-linux-gnu/glib-2.0/include \
        $$GSTREAMER_PATH/lib/x86_64-linux-gnu/gstreamer-1.0/include/
    #########################################################################
}

INCLUDEPATH += \
    $$GSTREAMER_PATH/include \
    $$GSTREAMER_PATH/include/gstreamer-1.0 \
    $$GSTREAMER_PATH/lib/gstreamer-1.0/include \
    $$GSTREAMER_PATH/include/glib-2.0 \
    $$GSTREAMER_PATH/lib/glib-2.0/include

LIBS += \
    -L$$GSTREAMER_PATH/lib \
    -lgio-2.0 \
    -lglib-2.0 \
    -lgobject-2.0 \
    -lgmodule-2.0 \
    -lgstreamer-1.0 \
    -lgstbase-1.0 \
    -lgstapp-1.0 \
    -lgstnet-1.0 \
    -lopencv_core \
    -lopencv_highgui

#-------------------------------------------------
    

