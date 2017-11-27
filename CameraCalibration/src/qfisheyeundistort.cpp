#include "qfisheyeundistort.h"

#include <opencv2/calib3d/calib3d.hpp>

using namespace std;

QFisheyeUndistort::QFisheyeUndistort(QObject *parent) : QObject(parent)
{
    mIntrinsic =  cv::Mat(3, 3, CV_64FC1 );
    mIntrinsic.ptr<double>(0)[0] = 1.0;
    mIntrinsic.ptr<double>(1)[1] = 1.0;
    mIntrinsic.ptr<double>(2)[2] = 1.0;

    mCoeffReady = false;
}


void QFisheyeUndistort::addCorners(vector<cv::Point2d>& img_corners, vector<cv::Point3d>& obj_corners, cv::Size imgSize )
{
    mMutex.lock();
    mObjCorners.push_back( obj_corners);
    mImgCorners.push_back( img_corners );


    vector<cv::Mat> rvecs;
    vector<cv::Mat> tvecs;

    int flags = 0;
    flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
    flags |= cv::fisheye::CALIB_CHECK_COND;
    flags |= cv::fisheye::CALIB_FIX_SKEW;

    cv::fisheye::calibrate( mObjCorners, mImgCorners, imgSize, mIntrinsic, mDistCoeffs, rvecs, tvecs, flags );

    mCoeffReady = true;

    mMutex.unlock();
}

cv::Mat QFisheyeUndistort::undistort(cv::Mat raw)
{
    if(!mCoeffReady)
        return cv::Mat();

    mMutex.lock();

    cv::Mat res;
    cv::fisheye::undistortImage( raw, res, mIntrinsic, mDistCoeffs );

    mMutex.unlock();

    return res;
}
