#include "qcameraundistort.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

QCameraUndistort::QCameraUndistort(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha,
                                   QObject *parent)
    : QObject(parent)
{
    mReady = setCameraParams( imgSize, fishEye, intr, dist, alpha );
}

bool QCameraUndistort::setCameraParams(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist , double alpha)
{
    mImgSize = imgSize;
    mFishEye = fishEye;

    mIntrinsic = intr;
    mDistCoeffs = dist;

    if( mIntrinsic.empty() || mDistCoeffs.empty() )
        return false;

    mAlpha = alpha;

    if( mFishEye )
    {
        if( mDistCoeffs.cols!=4)
            return false;

        cv::Mat newK;
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify( mIntrinsic, mDistCoeffs, mImgSize,
                                                                 newK, cv::noArray(),
                                                                 mAlpha );

        cv::fisheye::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                              newK, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }
    else
    {
        cv::Mat optimalK = cv::getOptimalNewCameraMatrix( mIntrinsic, mDistCoeffs, mImgSize, mAlpha );

        cv::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                     optimalK, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }

    mReady = true;
    return true;
}

cv::Mat QCameraUndistort::undistort(cv::Mat& raw )
{
    if( !mReady || mRemap1.empty() || mRemap2.empty() )
        return cv::Mat();

    cv::Mat res;
    cv::remap(raw, res, mRemap1, mRemap2, cv::INTER_LINEAR); // Apply undistorsion mappings

    return res;
}
