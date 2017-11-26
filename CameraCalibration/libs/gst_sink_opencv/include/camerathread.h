#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QMutex>
#include <QVector>

#include <opencv2/core/core.hpp>
#include "gst_sink_opencv.hpp"

#define MAT_BUF_SIZE 5

class CameraThread : public QThread
{
    Q_OBJECT

public:
    CameraThread();
    ~CameraThread();

signals:
    void newImage( cv::Mat frame );
    void cameraDisconnected();
    void cameraConnected();

protected:
    void run() Q_DECL_OVERRIDE;

private:
    GstSinkOpenCV* mImageSink;

    QVector<cv::Mat> mFrameBuffer;
    int mBufPushIdx;
    int mBufPopIdx;

    QMutex mFrameMutex;
};

#endif // CAMERATHREAD_H
