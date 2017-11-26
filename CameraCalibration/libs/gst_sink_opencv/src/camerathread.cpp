#include "camerathread.h"

#include <QDebug>
#include <string>

using namespace std;

CameraThread::CameraThread()
    : QThread(NULL)
    , mImageSink(NULL)
{
    qRegisterMetaType<cv::Mat>( "cv::Mat" );

    mBufPushIdx=0;
    mBufPopIdx=0;
    mFrameBuffer.resize(MAT_BUF_SIZE);
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

    mBufPushIdx=0;
    mBufPopIdx=0;

    forever
    {
        if( isInterruptionRequested() )
        {
            qDebug() << tr("CameraThread interruption requested");

            break;
        }

        //mFrameMutex.lock();

        mFrameBuffer[mBufPushIdx] = mImageSink->getLastFrame().clone();
        if( !mFrameBuffer[mBufPushIdx].empty() )
            mBufPushIdx = (mBufPushIdx+1)%MAT_BUF_SIZE;

        //mFrameMutex.unlock();

        if( !mFrameBuffer[mBufPopIdx].empty() )
        {
            emit newImage( mFrameBuffer[mBufPopIdx] );
            mBufPopIdx = (mBufPopIdx+1)%MAT_BUF_SIZE;
        }
    }

    if(mImageSink)
        delete mImageSink;

    qDebug() << tr("CameraThread stopped.");

    emit cameraDisconnected();
}
