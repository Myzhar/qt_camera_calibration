#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>

#include <opencv2/core/core.hpp>
#include "gst_sink_opencv.hpp"

class CameraThread : public QThread
{
    Q_OBJECT

public:
    CameraThread( double fps );
    ~CameraThread();

    double getBufPerc();

signals:
    void newImage( cv::Mat frame );
    void cameraDisconnected();
    void cameraConnected();



protected:
    void run() Q_DECL_OVERRIDE;

private:
    GstSinkOpenCV* mImageSink;

    double mFps;
};

#endif // CAMERATHREAD_H
