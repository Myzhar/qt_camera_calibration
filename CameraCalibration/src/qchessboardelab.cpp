#include "include/qchessboardelab.h"

#include <mainwindow.h>
#include <vector>

#include <opencv2/imgproc/imgproc.hpp>

#include "qfisheyeundistort.h"

using namespace std;

QChessboardElab::QChessboardElab( MainWindow* mainWnd, cv::Mat& frame, cv::Size cbSize, float cbSizeMm, QFisheyeUndistort* fisheyeUndist  )
    : QObject(NULL)
{
    mFrame = frame;
    mMainWnd = mainWnd;
    mCbSize = cbSize;
    mCbSizeMm = cbSizeMm;

    mFisheyeUndist = fisheyeUndist;

    connect( this, &QChessboardElab::newCbImage,
             mMainWnd, &MainWindow::onNewCbImage );
}

QChessboardElab::~QChessboardElab()
{
    disconnect( this, &QChessboardElab::newCbImage,
                mMainWnd, &MainWindow::onNewCbImage );
}

void QChessboardElab::run()
{
    cv::Mat gray;

    // >>>>> Chessboard detection
    cv::cvtColor( mFrame, gray,  CV_BGR2GRAY );
    vector<cv::Point2f> corners; //this will be filled by the detected corners

    //CALIB_CB_FAST_CHECK saves a lot of time on images
    //that do not contain any chessboard corners
    bool found = findChessboardCorners(gray, mCbSize, corners,
                                       cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE
                                       + cv::CALIB_CB_FAST_CHECK);

    if(found)
    {
        cv::cornerSubPix( gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                          cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

        cv::drawChessboardCorners( mFrame, mCbSize, cv::Mat(corners), found );

        vector<cv::Point3f> obj;

        int numSquares = mCbSize.width*mCbSize.height;
        for(int j=0;j<numSquares;j++)
        {
            obj.push_back(cv::Point3f((j/mCbSize.width)*mCbSizeMm, (j%mCbSize.width)*mCbSizeMm, 0.0f));
        }

        mFisheyeUndist->addCorners( corners, obj, mFrame.size() );
    }
    // <<<<< Chessboard detection


    emit newCbImage(mFrame);
}
