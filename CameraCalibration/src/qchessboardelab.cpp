#include "include/qchessboardelab.h"

#include <mainwindow.h>
#include <utility>
#include <vector>

#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>

#include "qcameracalibrate.h"

using namespace std;

QChessboardElab::QChessboardElab( MainWindow* mainWnd, cv::Mat& frame, cv::Size cbSize, float cbSizeMm, QCameraCalibrate* fisheyeUndist  )
    : QObject(nullptr)
{
    mFrame = frame;
    mMainWnd = mainWnd;
    mCbSize = std::move(cbSize);
    mCbSizeMm = cbSizeMm;

    mFisheyeUndist = fisheyeUndist;

    connect( this, &QChessboardElab::newCbImage,
             mMainWnd, &MainWindow::onNewCbImage );
    connect( this, &QChessboardElab::cbFound,
             mainWnd, &MainWindow::onCbDetected, Qt::BlockingQueuedConnection );
}

QChessboardElab::~QChessboardElab()
{
    disconnect( this, &QChessboardElab::newCbImage,
                mMainWnd, &MainWindow::onNewCbImage );

    disconnect( this, &QChessboardElab::cbFound,
             mMainWnd, &MainWindow::onCbDetected );
}

void QChessboardElab::run()
{
    try {
        cv::Mat gray;

        // >>>>> Chessboard detection
        cv::cvtColor(mFrame, gray, cv::COLOR_BGR2GRAY);
        vector<cv::Point2f> corners; //this will be filled by the detected corners

        //CALIB_CB_FAST_CHECK saves a lot of time on images
        //that do not contain any chessboard corners
        bool found = findChessboardCorners(gray, mCbSize, corners,
            cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE
            + cv::CALIB_CB_FAST_CHECK);

        if (found)
        {
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1));

            emit cbFound();

            cv::drawChessboardCorners(mFrame, mCbSize, cv::Mat(corners), found);

            vector<cv::Point3f> obj;

            int numSquares = mCbSize.width*mCbSize.height;
            obj.reserve(numSquares);
            for (int j = 0; j < numSquares; j++)
            {
                obj.emplace_back((j / mCbSize.width)*mCbSizeMm, (j%mCbSize.width)*mCbSizeMm, 0.0F);
            }

            mFisheyeUndist->addCorners(corners);
        }
        // <<<<< Chessboard detection
    }
    catch (const std::exception& ex) {
        qDebug() << __FUNCTION__ << ": exception: " << typeid(ex).name() << ": " << ex.what();
    }

    emit newCbImage(mFrame);
}
