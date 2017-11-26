message("Added qt_opencv_scene")

PATH = $$PWD
INC = $$PATH/include
SRC = $$PATH/src

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$INC

HEADERS += \
            $$INC/qopencvscene.h

SOURCES += \
            $$SRC/qopencvscene.cpp


    

