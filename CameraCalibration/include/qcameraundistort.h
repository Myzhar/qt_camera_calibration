#ifndef QCAMERAUNDISTORT_H
#define QCAMERAUNDISTORT_H

#include <QObject>

#include <opencv2/core/core.hpp>

class QCameraUndistort : public QObject
{
    Q_OBJECT
public:
    explicit QCameraUndistort(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha,
                              QObject *parent = nullptr);

    bool setCameraParams( cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha );

    cv::Mat undistort( cv::Mat& frame );

signals:

public slots:

private:
    cv::Size mImgSize;

    bool mFishEye;

    double mAlpha;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs;

    cv::Mat mRemap1;
    cv::Mat mRemap2;

    bool mReady;
};

#endif // QCAMERAUNDISTORT_H
