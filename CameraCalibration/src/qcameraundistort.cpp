#include "qcameraundistort.h"

#include <QtGlobal>

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

using namespace std;

QCameraUndistort::QCameraUndistort(cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, bool fishEye, int refineThreshm, QObject *parent)
    : QObject(parent)
{
    mImgSize = imgSize;
    mCbSize = cbSize;
    mCbSquareSizeMm = cbSquareSizeMm;

    mRefineThresh = refineThreshm;

    mFishEye = fishEye;

    mRefined = false;

    mReprojErr = NAN;

    mIntrinsic =  cv::Mat(3, 3, CV_64F, cv::Scalar::all(0.0f) );
    mIntrinsic.ptr<double>(0)[0] = 884.0;
    mIntrinsic.ptr<double>(1)[1] = 884.0;
    mIntrinsic.ptr<double>(2)[2] = 1.0;
    mIntrinsic.ptr<double>(0)[2] = (double)imgSize.width/2.0;
    mIntrinsic.ptr<double>(1)[2] = (double)imgSize.height/2.0;

    mDistCoeffs = cv::Mat( 8, 1, CV_64F, cv::Scalar::all(0.0f) );

    mCoeffReady = false;

    create3DChessboardCorners( mCbSize, mCbSquareSizeMm );
}

void QCameraUndistort::setCameraParams( cv::Mat& K, cv::Mat& D, bool fishEye )
{
    mFishEye = fishEye;

    mIntrinsic=K;
    mDistCoeffs=D;

    if(mFishEye)
    {
        // >>>>> FishEye model wants only 4 distorsion parameters
        cv::Mat feDist = cv::Mat( 4, 1, CV_64F, cv::Scalar::all(0.0f) );
        feDist.ptr<double>(0)[0] = mDistCoeffs.ptr<double>(0)[0];
        feDist.ptr<double>(1)[0] = mDistCoeffs.ptr<double>(1)[0];
        feDist.ptr<double>(2)[0] = mDistCoeffs.ptr<double>(2)[0];
        feDist.ptr<double>(3)[0] = mDistCoeffs.ptr<double>(3)[0];
        // <<<<< FishEye model wants only 4 distorsion parameters

        cv::fisheye::initUndistortRectifyMap( mIntrinsic, feDist, cv::Matx33f::eye(),
                                              mIntrinsic, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }
    else
    {
        cv::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                     mIntrinsic, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }

    mRefined=true;      // Initial guess is set, so we want to refine the calibration values
    mCoeffReady=true;

    //cout << "New K:" << endl << mIntrinsic << endl << endl;
    //cout << "New D:" << endl << mDistCoeffs << endl << endl;
}

void QCameraUndistort::addCorners( vector<cv::Point2f>& img_corners )
{
    mMutex.lock();

    if( mObjCornersVec.size() == mRefineThresh )
    {
        mObjCornersVec.clear();
        mImgCornersVec.clear();

        mRefined = true;
    }

    mObjCornersVec.push_back( mDefObjCorners );
    mImgCornersVec.push_back( img_corners );

    //if( mObjCornersVec.size() >= 5)
    {
        vector<cv::Mat> rvecs;
        vector<cv::Mat> tvecs;

        if(mFishEye)
        {
            // >>>>> Calibration flags
            mCalibFlags = cv::fisheye::CALIB_FIX_SKEW;
            if( mRefined )
            {
                mCalibFlags |= cv::fisheye::CALIB_USE_INTRINSIC_GUESS;
            }
            // <<<<< Calibration flags

            // >>>>> FishEye model wants only 4 distorsion parameters
            cv::Mat feDist = cv::Mat( 4, 1, CV_64F, cv::Scalar::all(0.0f) );
            feDist.ptr<double>(0)[0] = mDistCoeffs.ptr<double>(0)[0];
            feDist.ptr<double>(1)[0] = mDistCoeffs.ptr<double>(1)[0];
            feDist.ptr<double>(2)[0] = mDistCoeffs.ptr<double>(2)[0];
            feDist.ptr<double>(3)[0] = mDistCoeffs.ptr<double>(3)[0];
            // <<<<< FishEye model wants only 4 distorsion parameters

            mReprojErr = cv::fisheye::calibrate( mObjCornersVec, mImgCornersVec, mImgSize,
                                                 mIntrinsic, feDist, rvecs, tvecs, mCalibFlags );

            // >>>>> Update class distorsion matrix
            mDistCoeffs.ptr<double>(0)[0] = feDist.ptr<double>(0)[0];
            mDistCoeffs.ptr<double>(1)[0] = feDist.ptr<double>(1)[0];
            mDistCoeffs.ptr<double>(2)[0] = feDist.ptr<double>(2)[0];
            mDistCoeffs.ptr<double>(3)[0] = feDist.ptr<double>(3)[0];
            // <<<<< Update class distorsion matrix

            //cout << "Intrinsic before initUndistortRectifyMap: " << endl << mIntrinsic << endl << endl;

            cv::fisheye::initUndistortRectifyMap( mIntrinsic, feDist, cv::Matx33f::eye(),
                                                  mIntrinsic, mImgSize, CV_16SC2, mRemap1, mRemap2  );
        }
        else
        {
            // >>>>> Calibration flags
            mCalibFlags = CV_CALIB_RATIONAL_MODEL; // Using Camera model with 8 distorsion parameters
            if( mRefined )
            {
                mCalibFlags |= CV_CALIB_USE_INTRINSIC_GUESS;
            }
            // <<<<< Calibration flags

            mReprojErr = cv::calibrateCamera( mObjCornersVec, mImgCornersVec, mImgSize,
                                              mIntrinsic, mDistCoeffs, rvecs, tvecs, mCalibFlags );

            //cout << "Intrinsic before initUndistortRectifyMap: " << endl << mIntrinsic << endl << endl;

            cv::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                         mIntrinsic, mImgSize, CV_16SC2, mRemap1, mRemap2  );
        }

        //cv::Mat opt = cv::getOptimalNewCameraMatrix( mIntrinsic, mDistCoeffs, mImgSize, 1.0 );

        //cout << "Intrinsic after initUndistortRectifyMap: " << endl << mIntrinsic << endl << endl;

        emit newCameraParams( mIntrinsic, mDistCoeffs, mRefined, mReprojErr );

        mCoeffReady = true;
    }

    mMutex.unlock();
}

cv::Mat QCameraUndistort::undistort(cv::Mat raw)
{
    if(!mCoeffReady)
        return cv::Mat();

    mMutex.lock();

    cv::Mat res;
    cv::remap(raw, res, mRemap1, mRemap2, cv::INTER_LINEAR); // Apply undistorsion mappings

    mMutex.unlock();

    return res;
}

void QCameraUndistort::create3DChessboardCorners( cv::Size boardSize, double squareSize )
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
