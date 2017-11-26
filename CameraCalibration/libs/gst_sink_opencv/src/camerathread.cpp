#include "camerathread.h"

#include <QDebug>
#include <string>

using namespace std;

CameraThread::CameraThread()
    : QThread(NULL)
    , mImageSink(NULL)
{
    qRegisterMetaType<cv::Mat>( "cv::Mat" );

}

CameraThread::~CameraThread()
{
    if( isRunning() )
    {
        requestInterruption();

        if( !wait( 5000 ) )
        {
            qDebug() << tr("CameraThread not stopped. Forcing exit!");
        }
    }
}


void CameraThread::run()
{
#ifdef USE_ARM
    std::string pipeline = "udpsrc name=videosrc port=5000 ! "
                           "application/x-rtp,media=video,encoding-name=H264,pt=96 ! "
                           "rtph264depay ! h264parse ! omxh264dec";
#else
    std::string pipeline = "udpsrc name=videosrc port=5000 ! "
                           "application/x-rtp,media=video,encoding-name=H264,pt=96 ! "
                           "rtph264depay ! h264parse ! avdec_h264";
#endif

    mImageSink = GstSinkOpenCV::Create( pipeline, 3000 );

    if(!mImageSink)
    {
        emit cameraDisconnected();
        return;
    }

    emit cameraConnected();

    forever
    {
        if( isInterruptionRequested() )
        {
            qDebug() << tr("CameraThread interruption requested");

            break;
        }

        cv::Mat frame = mImageSink->getLastFrame().clone();

        if( !frame.empty() && !frame.rows==0 && !frame.cols==0 )
        {
            emit newImage( frame );
        }
    }

    if(mImageSink)
        delete mImageSink;

    qDebug() << tr("CameraThread stopped.");

    emit cameraDisconnected();
}
