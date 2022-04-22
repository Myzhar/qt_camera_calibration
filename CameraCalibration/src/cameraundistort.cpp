#include "cameraundistort.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <utility>

CameraUndistort::CameraUndistort(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha)
{
    mImgSize = imgSize;

    mIntrinsic =  cv::Mat(3, 3, CV_64F, cv::Scalar::all(0.0F) );
    mDistCoeffs = cv::Mat( 8, 1, CV_64F, cv::Scalar::all(0.0F) );

    mIntrinsic.ptr<double>(0)[0] = 1000.0;
    mIntrinsic.ptr<double>(1)[1] = 1000.0;
    mIntrinsic.ptr<double>(2)[2] = 1.0;
    mIntrinsic.ptr<double>(0)[2] = (double)mImgSize.width/2.0;
    mIntrinsic.ptr<double>(1)[2] = (double)mImgSize.height/2.0;


    mReady = setCameraParams( imgSize, fishEye, mIntrinsic, mDistCoeffs, alpha );
}

void CameraUndistort::getCameraParams( cv::Size& imgSize, bool& fishEye, cv::Mat& intr, cv::Mat& dist, double& alpha )
{
    imgSize = mImgSize;
    fishEye = mFishEye;
    intr = mIntrinsic;
    dist = mDistCoeffs;
    alpha = mAlpha;
}

bool CameraUndistort::setNewAlpha( double alpha )
{
    return setCameraParams( mImgSize, mFishEye, mIntrinsic, mDistCoeffs, alpha );
}

bool CameraUndistort::setFisheye(bool fisheye)
{
    return setCameraParams( mImgSize, fisheye, mIntrinsic, mDistCoeffs, mAlpha );
}

bool CameraUndistort::setCameraParams(cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist , double alpha)
{
    mImgSize = std::move(imgSize);
    mFishEye = fishEye;
    mAlpha = alpha;

    mIntrinsic = std::move(intr);
    mDistCoeffs = std::move(dist);

    if( mIntrinsic.empty() || mDistCoeffs.empty() ) {
        return false;    
}

    if( mFishEye )
    {
        // >>>>> FishEye model wants only 4 distorsion parameters
        cv::Mat feDist = cv::Mat( 4, 1, CV_64F, cv::Scalar::all(0.0F) );
        feDist.ptr<double>(0)[0] = mDistCoeffs.ptr<double>(0)[0];
        feDist.ptr<double>(1)[0] = mDistCoeffs.ptr<double>(1)[0];
        feDist.ptr<double>(2)[0] = mDistCoeffs.ptr<double>(2)[0];
        feDist.ptr<double>(3)[0] = mDistCoeffs.ptr<double>(3)[0];
        // <<<<< FishEye model wants only 4 distorsion parameters

        cv::Mat newK;
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify( mIntrinsic, feDist, mImgSize,
                                                                 cv::noArray(), newK,
                                                                 mAlpha );

        cv::fisheye::initUndistortRectifyMap( mIntrinsic, feDist, cv::Matx33f::eye(),
                                              newK, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }
    else
    {
        if( mDistCoeffs.rows!=8)
        {
            mDistCoeffs.resize(8, cv::Scalar::all(0.0));
        }

        cv::Mat optimalK = cv::getOptimalNewCameraMatrix( mIntrinsic, mDistCoeffs, mImgSize, mAlpha );

        cv::initUndistortRectifyMap( mIntrinsic, mDistCoeffs, cv::Matx33f::eye(),
                                     optimalK, mImgSize, CV_16SC2, mRemap1, mRemap2  );
    }

    mReady = true;
    return true;
}

bool CameraUndistort::saveCameraParams( std::string fileName )
{
    cv::FileStorage fs( fileName, cv::FileStorage::WRITE );

    if( !fs.isOpened() )
    {
        return false;
    }

    fs << "Width" << mImgSize.width;
    fs << "Height" << mImgSize.height;
    fs << "FishEye" << mFishEye;
    fs << "CameraMatrix" << mIntrinsic;
    fs << "DistCoeffs" << mDistCoeffs;

    return true;
}

bool CameraUndistort::loadCameraParams( std::string fileName )
{
    cv::FileStorage fs( fileName, cv::FileStorage::READ );

    if( !fs.isOpened() )
    {
        return false;
    }

    int w;
    int h;
    bool fisheye;
    cv::Mat intr;
    cv::Mat dist;
    double alpha;

    fs["Width"] >> w;
    fs["Height"] >> h;
    fs["FishEye"] >> fisheye;
    fs["Alpha"] >> alpha;

    fs["CameraMatrix"] >> intr;
    fs["DistCoeffs"] >> dist;

    if( w<1.0 || h<1.0 )
    {
        return false;
    }

    mImgSize.width = w;
    mImgSize.height = h;

    mFishEye = fisheye;

    mAlpha = alpha;

    if( intr.cols!=3 || intr.rows != 3 )
    {
        return false;
    }

    if( dist.cols != 1 )
    {
        return false;
    }

    if( !mFishEye && (dist.rows != 4 && dist.rows != 8 ) )
    {
        return false;
    }

    if( mDistCoeffs.rows!=8)
    {
        mDistCoeffs.resize(8, cv::Scalar::all(0.0));
    }

    intr.copyTo( mIntrinsic );
    dist.copyTo( mDistCoeffs );

    return true;
}

cv::Mat CameraUndistort::undistort(cv::Mat& raw )
{
    if( !mReady || mRemap1.empty() || mRemap2.empty() ) {
        return {};
}

    cv::Mat res;
    cv::remap(raw, res, mRemap1, mRemap2, cv::INTER_LINEAR); // Apply undistorsion mappings

    return res;
}
