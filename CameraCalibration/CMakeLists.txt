# Generated from CameraCalibration.pro.

cmake_minimum_required(VERSION 3.16)
project(CameraCalibration VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}")

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui OpenGL)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Multimedia Widgets)

FIND_PACKAGE( OpenCV 4 REQUIRED )

include(cmake/detect_gstreamer.cmake)

if (HAVE_GSTREAMER)
	message ("    GStreamer: (${GSTREAMER_VERSION})")
else()
	message( FATAL_ERROR "GStreamer not found." )
endif()



add_executable(CameraCalibration WIN32 MACOSX_BUNDLE
    include/cameraundistort.h
    include/mainwindow.h
    include/qcameracalibrate.h
    include/qchessboardelab.h
    include/v4l2compcamera.h
    libs/gst_sink_opencv/include/camerathread.h
    libs/gst_sink_opencv/include/gst_sink_opencv.hpp
    libs/gst_sink_opencv/src/camerathread.cpp
    libs/gst_sink_opencv/src/gst_sink_opencv.cpp
    libs/qt_opencv_scene/include/qopencvscene.h
    libs/qt_opencv_scene/src/qopencvscene.cpp
    src/cameraundistort.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/qcameracalibrate.cpp
    src/qchessboardelab.cpp
    src/v4l2compcamera.cpp
    src/mainwindow.ui
)
target_include_directories(CameraCalibration PUBLIC
    #/include
    #/include/glib-2.0
    #/include/gstreamer-1.0
    #/lib/glib-2.0/include
    #/lib/gstreamer-1.0/include
    ${GSTREAMER_INCLUDE_DIRS}
    OCV_INC_PATH
    include
    libs/gst_sink_opencv/include
    libs/qt_opencv_scene/include
)

target_link_libraries(CameraCalibration PUBLIC
    # Remove: L/lib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${OpenCV_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    #gio-2.0
    #glib-2.0
    #gmodule-2.0
    #gobject-2.0
    #gstapp-1.0
    #gstbase-1.0
    #gstnet-1.0
    #gstreamer-1.0
    #opencv_core
    #opencv_highgui
)

#target_compile_options(CameraCalibration
#    -std=c++11 -std=c++11
#)


# Resources:
set(resources_resource_files
    "sound/cell-phone-1-nr0.wav"
)

#qt6_add_resources(CameraCalibration "resources"
#    PREFIX
#        "/"
#    FILES
#        ${resources_resource_files}
#)

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(CameraCalibration PUBLIC
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::Widgets
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    target_link_libraries(CameraCalibration PUBLIC
        # Remove: L/usr/local/lib
        opencv_calib3d
        opencv_core
        opencv_features2d
        opencv_highgui
        opencv_imgproc
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_libraries(CameraCalibration PUBLIC
        # Remove: L/usr/local/lib
        opencv_calib3d
        opencv_core
        opencv_features2d
        opencv_highgui
        opencv_imgproc
    )
endif()

if(WIN32)
    target_link_libraries(CameraCalibration PUBLIC
        #ffi
        #intl
        ole32
        shlwapi
        winmm
        ws2_32
    )
endif()

if(LINUX)
    target_include_directories(CameraCalibration PUBLIC
        /usr/lib/x86_64-linux-gnu/glib-2.0/include
        /usr/lib/x86_64-linux-gnu/gstreamer-1.0/include
    )
endif()

install(TARGETS CameraCalibration
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
