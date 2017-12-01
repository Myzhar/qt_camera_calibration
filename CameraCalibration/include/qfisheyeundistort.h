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

    size_t getCbCount(){ return mImgCorners.size(); }

signals:

public slots:
    void addCorners(std::vector<cv::Point2f> &img_corners , std::vector<cv::Point3f> &obj_corners , cv::Size imgSize);    

private:
    QMutex mMutex;

    std::vector< std::vector<cv::Point2f> > mImgCorners;
    std::vector< std::vector<cv::Point3f> > mObjCorners;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs;

    bool mCoeffReady;
};

#endif // QFISHEYEUNDISTORT_H
