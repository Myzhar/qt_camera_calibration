#include "qfisheyeundistort.h"

#include <opencv2/calib3d/calib3d.hpp>

#include <iostream>

using namespace std;

QFisheyeUndistort::QFisheyeUndistort(cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, QObject *parent)
    : QObject(parent)
{
    mImgSize = imgSize;
    mCbSize = cbSize;
    mCbSquareSizeMm = cbSquareSizeMm;

    mIntrinsic =  cv::Mat(3, 3, CV_32F, cv::Scalar::all(0.0f) );
    mIntrinsic.ptr<float>(0)[0] = 1.0;
    mIntrinsic.ptr<float>(1)[1] = 1.0;
    mIntrinsic.ptr<float>(2)[2] = 1.0;
    mIntrinsic.ptr<float>(0)[2] = imgSize.width/2;
    mIntrinsic.ptr<float>(1)[2] = imgSize.height/2;

    cout << "Initial intrinsic: " << endl << mIntrinsic << endl;;

    mDistCoeffs = cv::Mat( 4, 1, CV_32F, cv::Scalar::all(0.0f) );

    mCoeffReady = false;

    create3DChessboardCorners( mCbSize, mCbSquareSizeMm );
}

void QFisheyeUndistort::addCorners(vector<cv::Point2f>& img_corners)
{
    mMutex.lock();
    mObjCornersVec.push_back( mDefObjCorners );
    mImgCornersVec.push_back( img_corners );

    if( mObjCornersVec.size() > 5)
    {
        vector<cv::Mat> rvecs;
        vector<cv::Mat> tvecs;

        int flags = 0;
        flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
        flags |= cv::fisheye::CALIB_CHECK_COND;
        flags |= cv::fisheye::CALIB_FIX_SKEW;

        cv::fisheye::calibrate( mObjCornersVec, mImgCornersVec, mImgSize, mIntrinsic, mDistCoeffs, rvecs, tvecs, flags );

        mCoeffReady = true;
    }

    mMutex.unlock();
}

cv::Mat QFisheyeUndistort::undistort(cv::Mat raw)
{
    if(!mCoeffReady)
        return cv::Mat();

    //cout << "Intrinsic: " << endl << mIntrinsic << endl << endl;
    //cout << "Distorsion: " << endl << mDistCoeffs << endl << endl;

    cv::Mat identity = cv::Mat::eye(3, 3, CV_32F);

    //mMutex.lock();

    cv::Mat res;
    cv::fisheye::undistortImage( raw, res, mIntrinsic, mDistCoeffs, identity );

    //mMutex.unlock();

    return res;
}

void QFisheyeUndistort::create3DChessboardCorners(cv::Size boardSize, float squareSize)
{
    // This function creates the 3D points of your chessboard in its own coordinate system
    float width = (boardSize.width-1)*squareSize;
    float height = (boardSize.height-1)*squareSize;

    mDefObjCorners.clear();

    for( int i = 0; i < boardSize.height; i++ )
    {
        for( int j = 0; j < boardSize.width; j++ )
        {
            mDefObjCorners.push_back(cv::Point3d(float(j*squareSize)-width, float(i*squareSize)-height, 0));
        }
    }
}
