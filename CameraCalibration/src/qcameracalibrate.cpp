#include "qcameracalibrate.h"

#include <QtGlobal>
#include <QDebug>
#include <QMutexLocker>

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <utility>

#include "cameraundistort.h"

using namespace std;

QCameraCalibrate::QCameraCalibrate(cv::Size imgSize, cv::Size cbSize, float cbSquareSizeMm, bool fishEye, int refineThreshm, QObject *parent)
    : QObject(parent)
    , mUndistort(nullptr)
{
    mImgSize = std::move(imgSize);
    mCbSize = std::move(cbSize);
    mCbSquareSizeMm = cbSquareSizeMm;
    //mAlpha = 0.0;

    mRefineThresh = refineThreshm;

    //mFishEye = fishEye;

    mRefined = false;

    mReprojErr = NAN;

    mCoeffReady = false;

    create3DChessboardCorners( mCbSize, mCbSquareSizeMm );

    mUndistort = new CameraUndistort( mImgSize );
}

QCameraCalibrate::~QCameraCalibrate()
{
    delete mUndistort;
}

void QCameraCalibrate::setNewAlpha( double alpha )
{
    if( mUndistort )
    {
        mUndistort->setNewAlpha(alpha);
    }
}

void QCameraCalibrate::setFisheye( bool fisheye )
{
    if( mUndistort )
    {
        mUndistort->setFisheye(fisheye);
    }
}

void QCameraCalibrate::getCameraParams( cv::Size& imgSize, cv::Mat &K, cv::Mat &D, double &alpha, bool &fisheye)
{
    if( !mUndistort ) {
        return;
    }

    mUndistort->getCameraParams( imgSize, fisheye, K, D, alpha );
}

bool QCameraCalibrate::setCameraParams(cv::Size imgSize, cv::Mat &K, cv::Mat &D, double alpha, bool fishEye )
{
    if( !mUndistort ) {
        return false;
    }

    mImgSize = std::move(imgSize);

    if( mUndistort->setCameraParams( mImgSize, fishEye, K, D, alpha ) )
    {
        mRefined=true;      // Initial guess is set, so we want to refine the calibration values
        mCoeffReady=true;

        return true;
    }

    mRefined = false;
    mCoeffReady = false;

    return false;
}

void QCameraCalibrate::addCorners( vector<cv::Point2f>& img_corners )
{
    QMutexLocker locker(&mMutex);

    if( (int)mObjCornersVec.size() == mRefineThresh )
    {
        mObjCornersVec.clear();
        mImgCornersVec.clear();

        mRefined = true;
    }

    mObjCornersVec.push_back( mDefObjCorners );
    mImgCornersVec.push_back( img_corners );

    if( mObjCornersVec.size() >= 5)
    {
        vector<cv::Mat> rvecs;
        vector<cv::Mat> tvecs;

        cv::Size imgSize;
        bool fisheye;
        cv::Mat K;
        cv::Mat D;
        double alpha;

        mUndistort->getCameraParams( imgSize, fisheye, K, D, alpha );

        if(fisheye)
        {
            // >>>>> Calibration flags
            int mCalibFlags = cv::fisheye::CALIB_FIX_SKEW;
            if( mRefined )
            {
                mCalibFlags |= cv::fisheye::CALIB_USE_INTRINSIC_GUESS;
            }
            // <<<<< Calibration flags

            // >>>>> FishEye model wants only 4 distorsion parameters
            cv::Mat feDist = cv::Mat( 4, 1, CV_64F, cv::Scalar::all(0.0F) );
            feDist.ptr<double>(0)[0] = D.ptr<double>(0)[0];
            feDist.ptr<double>(1)[0] = D.ptr<double>(1)[0];
            feDist.ptr<double>(2)[0] = D.ptr<double>(2)[0];
            feDist.ptr<double>(3)[0] = D.ptr<double>(3)[0];
            // <<<<< FishEye model wants only 4 distorsion parameters

            mReprojErr = cv::fisheye::calibrate( mObjCornersVec, mImgCornersVec, mImgSize,
                                                 K, feDist, rvecs, tvecs, mCalibFlags );

            // >>>>> Update class distorsion matrix
            D.ptr<double>(0)[0] = feDist.ptr<double>(0)[0];
            D.ptr<double>(1)[0] = feDist.ptr<double>(1)[0];
            D.ptr<double>(2)[0] = feDist.ptr<double>(2)[0];
            D.ptr<double>(3)[0] = feDist.ptr<double>(3)[0];
            // <<<<< Update class distorsion matrix

            mUndistort->setCameraParams( mImgSize, fisheye, K, D, alpha );
        }
        else
        {
            // >>>>> Calibration flags
            int mCalibFlags = cv::CALIB_RATIONAL_MODEL; // Using Camera model with 8 distorsion parameters
            if( mRefined )
            {
                mCalibFlags |= cv::CALIB_USE_INTRINSIC_GUESS;
            }
            // <<<<< Calibration flags

            mReprojErr = cv::calibrateCamera( mObjCornersVec, mImgCornersVec, mImgSize,
                                              K, D, rvecs, tvecs, mCalibFlags );

            mUndistort->setCameraParams( mImgSize, fisheye, K, D, alpha );
        }

        emit newCameraParams( K, D, mRefined, mReprojErr );

        mCoeffReady = true;
    }
}

cv::Mat QCameraCalibrate::undistort(cv::Mat& raw)
{
    if(!mCoeffReady) {
        return {};
    }

    QMutexLocker locker(&mMutex);

    cv::Mat res;

    if(mUndistort)
    {
        res = mUndistort->undistort( raw );
    }

    return res;
}

void QCameraCalibrate::create3DChessboardCorners( cv::Size boardSize, double squareSize )
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
