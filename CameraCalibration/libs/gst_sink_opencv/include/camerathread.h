#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include "camerathreadbase.h"

#include <QMutex>
#include <QQueue>

#include <opencv2/core/core.hpp>
#include "gst_sink_opencv.hpp"

class CameraThread : public CameraThreadBase
{
    Q_OBJECT

public:
    CameraThread( double fps );
    ~CameraThread();

    double getBufPerc() override;
    void dataConsumed() override {}

signals:
    void newImage( cv::Mat frame );
    void cameraDisconnected();
    void cameraConnected();

protected:
    void run() override;

private:
    GstSinkOpenCV* mImageSink;

    double mFps;
};

#endif // CAMERATHREAD_H
