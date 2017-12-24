#include "qcameraundistort.h"
#include <opencv2/calib3d/calib3d.hpp>

QCameraUndistort::QCameraUndistort(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha,
                                   QObject *parent)
    : QObject(parent)
{
    setCameraParams( imgSize, fishEye, intr, dist, alpha );
}

bool QCameraUndistort::setCameraParams(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist , double alpha)
{
    mFrmSize = imgSize;
    mFishEye = fishEye;

    mIntrinsic = intr;
    mDistorsion = dist;

    mAlpha = alpha;

    if( mFishEye )
    {
        cv::fisheye::initUndistortRectifyMap( mIntrinsic, mDistorsion, cv::Matx33f::eye(),
                                              nk, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }
    else
    {

    }
}
