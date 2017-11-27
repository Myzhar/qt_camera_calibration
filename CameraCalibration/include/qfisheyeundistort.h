#ifndef QFISHEYEUNDISTORT_H
#define QFISHEYEUNDISTORT_H

#include <QObject>
#include <QMutex>
#include <opencv2/core/core.hpp>

#include <vector>

class QFisheyeUndistort : public QObject
{
    Q_OBJECT
public:
    explicit QFisheyeUndistort(QObject *parent = nullptr);

    cv::Mat undistort(cv::Mat raw);


signals:

public slots:
    void addCorners(std::vector<cv::Point2d> &img_corners , std::vector<cv::Point3d> &obj_corners , cv::Size imgSize);

private:
    QMutex mMutex;

    std::vector< std::vector<cv::Point2d> > mImgCorners;
    std::vector< std::vector<cv::Point3d> > mObjCorners;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs;

    bool mCoeffReady;
};

#endif // QFISHEYEUNDISTORT_H
