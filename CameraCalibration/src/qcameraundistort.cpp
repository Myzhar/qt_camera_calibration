#include "qfisheyeundistort.h"

#include <QtGlobal>

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

using namespace std;

QFisheyeUndistort::QFisheyeUndistort(cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, QObject *parent)
    : QObject(parent)
{
    mImgSize = imgSize;
    mCbSize = cbSize;
    mCbSquareSizeMm = cbSquareSizeMm;

    mRefined = false;

    mReprojErr = NAN;

    mIntrinsic =  cv::Mat(3, 3, CV_64F, cv::Scalar::all(0.0f) );
    mIntrinsic.ptr<double>(0)[0] = 884.0;
    mIntrinsic.ptr<double>(1)[1] = 884.0;
    mIntrinsic.ptr<double>(2)[2] = 1.0;
    mIntrinsic.ptr<double>(0)[2] = (double)imgSize.width/2.0;
    mIntrinsic.ptr<double>(1)[2] = (double)imgSize.height/2.0;

    mDistCoeffs = cv::Mat( 4, 1, CV_64F, cv::Scalar::all(0.0f) );

    mCalibFlags = cv::fisheye::CALIB_FIX_SKEW;

    mCoeffReady = false;

    create3DChessboardCorners( mCbSize, mCbSquareSizeMm );
}

void QFisheyeUndistort::addCorners(vector<cv::Point2f>& img_corners)
{
    mMutex.lock();

    if( mObjCornersVec.size() == 15 )
    {
        mObjCornersVec.clear();
        mImgCornersVec.clear();

        mCalibFlags |= cv::fisheye::CALIB_USE_INTRINSIC_GUESS;
        mRefined = true;
    }

    mObjCornersVec.push_back( mDefObjCorners );
    mImgCornersVec.push_back( img_corners );

    if( mObjCornersVec.size() >= 5)
    {
        vector<cv::Mat> rvecs;
        vector<cv::Mat> tvecs;

        cv::fisheye::calibrate( mObjCornersVec, mImgCornersVec, mImgSize, mIntrinsic, mDistCoeffs, rvecs, tvecs, mCalibFlags );

        //cv::Mat opt = cv::getOptimalNewCameraMatrix( mIntrinsic, mDistCoeffs, mImgSize, 1.0 );

        cv::fisheye::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                              mIntrinsic, mImgSize, CV_16SC2, mRemap1, mRemap2  );

        emit newCameraParams( mIntrinsic, mDistCoeffs, mRefined );

        mCoeffReady = true;
    }

    mMutex.unlock();
}

cv::Mat QFisheyeUndistort::undistort(cv::Mat raw)
{
    if(!mCoeffReady)
        return cv::Mat();

    mMutex.lock();

    cv::Mat res;
    cv::remap(raw, res, mRemap1, mRemap2, cv::INTER_LINEAR); // Apply undistorsion mappings

    mMutex.unlock();

    return res;
}

void QFisheyeUndistort::create3DChessboardCorners( cv::Size boardSize, double squareSize )
{
    // This function creates the 3D points of your chessboard in its own coordinate system
    double width = (boardSize.width-1)*squareSize;
    double height = (boardSize.height-1)*squareSize;

    mDefObjCorners.clear();

    for( int i = 0; i < boardSize.height; i++ )
    {
        for( int j = 0; j < boardSize.width; j++ )
        {
            mDefObjCorners.push_back(cv::Point3d(double(j*squareSize)-width, double(i*squareSize)-height, 0.0));
        }
    }
}
