#ifndef QCHESSBOARDELAB_H
#define QCHESSBOARDELAB_H

#include <QObject>
#include <QRunnable>
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>

class MainWindow;
class QFisheyeUndistort;

class QChessboardElab : public QObject, public QRunnable
{
    Q_OBJECT

public:
    QChessboardElab(MainWindow* mainWnd, cv::Mat& frame, cv::Size cbSize, double cbSizeMm  , QFisheyeUndistort *fisheyeUndist);
    virtual ~QChessboardElab();

    virtual void run();

signals:
    void newCbImage( cv::Mat cbImage );

private:
    cv::Mat mFrame;
    cv::Size mCbSize;
    double mCbSizeMm;

    MainWindow* mMainWnd;
    QFisheyeUndistort* mFisheyeUndist;

};

#endif // QCHESSBOARDELAB_H
