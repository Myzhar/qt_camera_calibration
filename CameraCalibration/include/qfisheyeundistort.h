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
    explicit QFisheyeUndistort( cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, QObject *parent = nullptr );

    cv::Mat undistort(cv::Mat raw);

    size_t getCbCount(){ return mImgCornersVec.size(); }

protected:
    void create3DChessboardCorners(cv::Size boardSize, float squareSize);

signals:

public slots:
    void addCorners(std::vector<cv::Point2f> &img_corners );

private:
    QMutex mMutex;

    std::vector< std::vector<cv::Point2f> > mImgCornersVec;
    std::vector< std::vector<cv::Point3f> > mObjCornersVec;

    std::vector<cv::Point3f> mDefObjCorners;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs;

    bool mCoeffReady;

    cv::Size mImgSize;
    cv::Size mCbSize;
    float mCbSquareSizeMm;
};

#endif // QFISHEYEUNDISTORT_H
