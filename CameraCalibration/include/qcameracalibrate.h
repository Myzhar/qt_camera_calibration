#ifndef QFISHEYEUNDISTORT_H
#define QFISHEYEUNDISTORT_H

#include <QObject>
#include <QMutex>
#include <opencv2/core/core.hpp>

#include <vector>

class QCameraUndistort;

class QCameraCalibrate : public QObject
{
    Q_OBJECT
public:
    explicit QCameraCalibrate(cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, bool fishEye,
                              int refineThreshm = 10, QObject *parent = nullptr );

    virtual ~QCameraCalibrate();

    cv::Mat undistort(cv::Mat &raw);

    size_t getCbCount()
    {
        return mImgCornersVec.size();
    }

    void getCameraParams(cv::Mat& K, cv::Mat& D)
    {
        K=mIntrinsic;
        D=mDistCoeffs;
    }

    void setCameraParams(cv::Mat& K, cv::Mat& D, bool fishEye);
    void setNewAlpha( double alpha );

protected:
    void create3DChessboardCorners(cv::Size boardSize, double squareSize);

signals:
    void newCameraParams(cv::Mat K, cv::Mat D, bool refined, double reprojErr );

public slots:
    void addCorners(std::vector<cv::Point2f> &img_corners );

private:
    QMutex mMutex;

    std::vector< std::vector<cv::Point2f> > mImgCornersVec;
    std::vector< std::vector<cv::Point3f> > mObjCornersVec;

    std::vector<cv::Point3f> mDefObjCorners;

    int mCalibFlags;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs;

    cv::Mat mRemap1;
    cv::Mat mRemap2;

    bool mCoeffReady;
    bool mRefined;
    bool mFishEye;
    double mAlpha;

    cv::Size mImgSize;
    cv::Size mCbSize;
    float mCbSquareSizeMm;

    double mReprojErr;

    int mRefineThresh;

    QCameraUndistort* mUndistort;
};

#endif // QFISHEYEUNDISTORT_H
