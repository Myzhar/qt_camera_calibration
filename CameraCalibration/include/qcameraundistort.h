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

signals:

public slots:

private:
    cv::Size mFrmSize;

    bool mFishEye;

    double mAlpha;

    cv::Mat mIntrinsic;
    cv::Mat mDistorsion;

    cv::Mat mRemap1;
    cv::Mat mRemap2;
};

#endif // QCAMERAUNDISTORT_H
