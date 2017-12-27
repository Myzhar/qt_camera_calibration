#ifndef QCAMERAUNDISTORT_H
#define QCAMERAUNDISTORT_H

#include <opencv2/core/core.hpp>
#include <string>

class CameraUndistort
{
public:
    explicit CameraUndistort( cv::Size imgSize, bool fishEye=false,
                              cv::Mat intr=cv::Mat(), cv::Mat dist=cv::Mat(),
                              double alpha=0.0 );

    bool setCameraParams( cv::Size imgSize, bool fishEye, cv::Mat intr, cv::Mat dist, double alpha );
    void getCameraParams( cv::Size& imgSize, bool& fishEye, cv::Mat& intr, cv::Mat& dist, double& alpha );

    bool loadCameraParams( std::string fileName );
    bool saveCameraParams( std::string fileName );

    bool setNewAlpha( double alpha );
    bool setFisheye( bool fisheye );

    cv::Mat undistort( cv::Mat& frame );

private:
    cv::Size mImgSize;

    bool mFishEye;

    double mAlpha;

    cv::Mat mIntrinsic;
    cv::Mat mDistCoeffs; // 4x1 if FishEye, 4x1 or 8x1 if not Fisheye

    cv::Mat mRemap1;
    cv::Mat mRemap2;

    bool mReady;
};

#endif // QCAMERAUNDISTORT_H
