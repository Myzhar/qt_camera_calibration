###########################################
# Add Custom OpenCV library

OCV_PATH = /usr/local

OCV_LIBS_PATH = $$OCV_PATH/lib
OCV_INC_PATH  = $$OCV_PATH/include/

INCLUDEPATH += OCV_INC_PATH

CONFIG(release, debug|release) {

    message("Added OpenCV Release")

    LIBS += \
            -L$$OCV_LIBS_PATH \
            -lopencv_core \
            -lopencv_imgproc \
            -lopencv_highgui \
            -lopencv_calib3d \
            -lopencv_features2d
}

CONFIG(debug, debug|release) {

    message("Added OpenCV Debug")

    LIBS += \
            -L$$OCV_LIBS_PATH \
            -lopencv_core \
            -lopencv_imgproc \
            -lopencv_highgui \
            -lopencv_calib3d \
            -lopencv_features2d
}
