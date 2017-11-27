#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "camerathread.h"
#include "qopencvscene.h"

#include <QCameraInfo>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <vector>

#include "qchessboardelab.h"
#include "qfisheyeundistort.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mCameraThread(NULL),
    mCameraSceneRaw(NULL),
    mCameraSceneCheckboard(NULL),
    mCameraSceneUndistorted(NULL),
    mFisheyeUndist(NULL)
{
    ui->setupUi(this);

    mCameraConnected = false;

    updateOpenCvVer();

    // >>>>> OpenCV version
    QString ocvVers = updateOpenCvVer();
    mOpenCvVer.setText( ocvVers );
    ui->statusBar->addPermanentWidget( &mOpenCvVer );
    // <<<<< OpenCV version

    on_pushButton_update_camera_list_clicked();

    // >>>>> Stream rendering
    mCameraSceneRaw = new QOpenCVScene();
    mCameraSceneCheckboard = new QOpenCVScene();
    mCameraSceneUndistorted = new QOpenCVScene();

    ui->graphicsView_raw->setScene( mCameraSceneRaw );
    ui->graphicsView_raw->setBackgroundBrush( QBrush( QColor(200,50,50) ) );

    ui->graphicsView_checkboard->setScene( mCameraSceneCheckboard );
    ui->graphicsView_checkboard->setBackgroundBrush( QBrush( QColor(50,200,50) ) );

    ui->graphicsView_undistorted->setScene( mCameraSceneUndistorted );
    ui->graphicsView_undistorted->setBackgroundBrush( QBrush( QColor(50,50,200) ) );

    // <<<<< Stream rendering

    mElabPool.setMaxThreadCount( 3 );
}

MainWindow::~MainWindow()
{
    while( mGstProcess.state() == QProcess::Running )
    {
        mGstProcess.kill();
        QApplication::processEvents( QEventLoop::AllEvents, 50 );
    }

    mElabPool.clear();

    delete ui;

    if(mCameraThread)
        delete mCameraThread;

    if(mCameraSceneRaw)
        delete mCameraSceneRaw;

    if(mCameraSceneCheckboard)
        delete mCameraSceneCheckboard;

    if(mCameraSceneUndistorted)
        delete mCameraSceneUndistorted;

    if(mFisheyeUndist)
        delete mFisheyeUndist;
}

QString MainWindow::updateOpenCvVer()
{
    QString ocvVers = tr("OpenCV %1.%2.%3").arg(CV_MAJOR_VERSION).arg(CV_MINOR_VERSION).arg(CV_SUBMINOR_VERSION);

    return ocvVers;
}

QStringList MainWindow::updateCameraInfo()
{
    QStringList res;

    mCameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, mCameras)
    {
        QString name = cameraInfo.deviceName();
        qDebug() << cameraInfo.description();

        res.push_back(name);
    }

    return res;
}

void MainWindow::on_pushButton_update_camera_list_clicked()
{
    ui->comboBox_camera->clear();
    ui->comboBox_camera->addItems( updateCameraInfo() );
}

void MainWindow::on_comboBox_camera_currentIndexChanged(int index)
{
    if( mCameras.size()<1 )
        return;

    if( index>mCameras.size()-1  )
        return;

    ui->label_camera->setText( static_cast<QCameraInfo>(mCameras.at(index)).description() );
}

bool MainWindow::startCamera()
{
    if(!startGstProcess())
        return false;

    if( mCameraThread )
    {
        delete mCameraThread;
        mCameraThread = NULL;
    }

    mCameraThread = new CameraThread();

    connect( mCameraThread, &CameraThread::cameraConnected,
             this, &MainWindow::onCameraConnected );
    connect( mCameraThread, &CameraThread::cameraDisconnected,
             this, &MainWindow::onCameraDisconnected );
    connect( mCameraThread, &CameraThread::newImage,
             this, &MainWindow::onNewImage );

    mCameraThread->start();

    return true;
}

void MainWindow::stopCamera()
{
    if( mCameraThread )
    {
        disconnect( mCameraThread, &CameraThread::cameraConnected,
                    this, &MainWindow::onCameraConnected );
        disconnect( mCameraThread, &CameraThread::cameraDisconnected,
                    this, &MainWindow::onCameraDisconnected );
        disconnect( mCameraThread, &CameraThread::newImage,
                    this, &MainWindow::onNewImage );

        delete mCameraThread;
        mCameraThread = NULL;
    }
}

void MainWindow::onCameraConnected()
{
    mCameraConnected = true;
}

void MainWindow::onCameraDisconnected()
{
    mCameraConnected = false;
}

void MainWindow::onNewImage( cv::Mat frame )
{

    static int frameW = 0;
    static int frameH = 0;

    if( frameW != frame.cols ||
            frameH != frame.rows)
    {
        ui->graphicsView_raw->fitInView(QRectF(0,0, frame.cols, frame.rows),
                                        Qt::KeepAspectRatio );
        ui->graphicsView_checkboard->fitInView(QRectF(0,0, frame.cols, frame.rows),
                                               Qt::KeepAspectRatio );
        ui->graphicsView_undistorted->fitInView(QRectF(0,0, frame.cols, frame.rows),
                                                Qt::KeepAspectRatio );
        frameW = frame.cols;
        frameH = frame.rows;
    }

    mCameraSceneRaw->setFgImage(frame);

    QChessboardElab* elab = new QChessboardElab( this, frame, mCbSize, mCbSizeMm, mFisheyeUndist );
    //mElabPool.start( elab );
    mElabPool.tryStart(elab);

    cv::Mat rectified = mFisheyeUndist->undistort( frame );

    if( rectified.empty() )
    {
        mCameraSceneUndistorted->setFgImage(frame);
    }
    else
    {
        mCameraSceneUndistorted->setFgImage(rectified);
    }

}


void MainWindow::onNewCbImage(cv::Mat cbImage)
{
    mCameraSceneCheckboard->setFgImage(cbImage);
}

void MainWindow::on_pushButton_camera_connect_disconnect_clicked(bool checked)
{
    if( checked )
    {
        mCamDev = ui->comboBox_camera->currentText();
        mSrcWidth = ui->lineEdit_camera_w->text().toInt();
        mSrcHeight = ui->lineEdit_camera_h->text().toInt();
        mSrcFps = ui->lineEdit_camera_fps->text().toInt();

        mCbSize.width = ui->lineEdit_chessboard_cols->text().toInt();
        mCbSize.height = ui->lineEdit__chessboard_rows->text().toInt();
        mCbSizeMm = ui->lineEdit__chessboard_mm->text().toDouble();

        if(mFisheyeUndist)
        {
            delete mFisheyeUndist;
        }

        mFisheyeUndist = new QFisheyeUndistort();

        if( startCamera() )
        {
            ui->pushButton_camera_connect_disconnect->setText( tr("Stop Camera") );
        }
        else
        {
            ui->pushButton_camera_connect_disconnect->setText( tr("Start Camera") );
            ui->pushButton_camera_connect_disconnect->setChecked(false);
        }
    }
    else
    {
        ui->pushButton_camera_connect_disconnect->setText( tr("Start Camera") );
        stopCamera();
    }
}

void MainWindow::onProcessReadyRead()
{
    while( mGstProcess.bytesAvailable() )
    {
        QByteArray line = mGstProcess.readLine();

        qDebug() << line;

        QApplication::processEvents( QEventLoop::AllEvents, 5 );
    }
}

bool MainWindow::startGstProcess( )
{
    if( mCamDev.size()==0 )
        return false;

    // >>>>> Kill gst-launch-1.0 processes
    QProcess killer;
    QProcess checker;

    int count = 0;

    bool done = false;
    do
    {
        killer.start( "pkill gst-launch" );
        killer.waitForFinished( 1000 );

        checker.start( "pgrep gst-launch" );
        checker.waitForFinished( 1000 );

        done = checker.readAll().size()==0;
        count++;

        if( count==10 )
        {
            qDebug() << tr("Cannot kill gst-launch active process(es)" );
            // TODO add error message
            break;
        }

    } while( !done );
    // <<<<< Kill gst-launch-1.0 processes

    QString launchStr;

#ifdef USE_ARM
    launchStr = tr(
                "gst-launch-1.0 v4l2src device=%1 do-timestamp=true ! "
                "\"video/x-raw,format=I420,width=%2,height=%3,framerate=%4/1\" ! nvvidconv ! "
                "\"video/x-raw(memory:NVMM),width=%2,height=%3\" ! "
                //"omxh264enc low-latency=true insert-sps-pps=true ! "
                "omxh264enc insert-sps-pps=true ! "
                "rtph264pay config-interval=1 pt=96 mtu=9000 ! queue ! "
                "udpsink host=127.0.0.1 port=5000 sync=false async=false -e"
                ).arg(mCamDev).arg(mSrcWidth).arg(mSrcHeight).arg(mSrcFps);
#else
    launchStr =
            tr("gst-launch-1.0 v4l2src device=%1 ! "
               "\"video/x-raw,format=I420,width=%2,height=%3,framerate=%4/1\" ! videoconvert ! "
               //"videoscale ! \"video/x-raw,width=%5,height=%6\" ! "
               "x264enc key-int-max=1 tune=zerolatency bitrate=8000 ! "
               "rtph264pay config-interval=1 pt=96 mtu=9000 ! queue ! "
               "udpsink host=127.0.0.1 port=5000 sync=false async=false -e").arg(mCamDev).arg(mSrcWidth).arg(mSrcHeight).arg(mSrcFps);
#endif

    qDebug() << tr("Starting pipeline: \n %1").arg(launchStr);

    mGstProcess.setProcessChannelMode( QProcess::MergedChannels );
    mGstProcess.start( launchStr );

    if( !mGstProcess.waitForStarted( 10000 ) )
    {
        // TODO Camera error message

        return false;
    }

    return true;
}
