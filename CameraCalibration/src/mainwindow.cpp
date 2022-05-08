#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "camerathread.h"
#include "qopencvscene.h"

#include <QCameraInfo>
#include <QGLWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QSound>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <vector>

#include "qchessboardelab.h"
#include "qcameracalibrate.h"

#include <iostream>


using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mCameraThread(nullptr),
    mCameraSceneRaw(nullptr),
    mCameraSceneCheckboard(nullptr),
    mCameraSceneUndistorted(nullptr),
    mCameraCalib(nullptr),
    mCbDetectedSnd(nullptr)
{
    ui->setupUi(this);

    killGstLaunch();

    mCameraConnected = false;

    mCbDetectedSnd = new QSound( "://sound/cell-phone-1-nr0.wav", this);

    updateOpenCvVer();

    // >>>>> OpenCV version
    QString ocvVers = updateOpenCvVer();
    mOpenCvVer.setText( ocvVers );
    ui->statusBar->addPermanentWidget( &mOpenCvVer );
    // <<<<< OpenCV version

    // >>>>> Calibration INFO
    ui->statusBar->addWidget( &mCalibInfo );
    // <<<<< Calibration INFO

    on_pushButton_update_camera_list_clicked();

    // >>>>> Stream rendering
    mCameraSceneRaw = new QOpenCVScene();
    mCameraSceneCheckboard = new QOpenCVScene();
    mCameraSceneUndistorted = new QOpenCVScene();

    ui->graphicsView_raw->setViewport( new QGLWidget );
    ui->graphicsView_raw->setScene( mCameraSceneRaw );
    ui->graphicsView_raw->setBackgroundBrush( QBrush( QColor(100,100,100) ) );

    ui->graphicsView_checkboard->setViewport( new QGLWidget );
    ui->graphicsView_checkboard->setScene( mCameraSceneCheckboard );
    ui->graphicsView_checkboard->setBackgroundBrush( QBrush( QColor(50,150,150) ) );

    ui->graphicsView_undistorted->setViewport( new QGLWidget );
    ui->graphicsView_undistorted->setScene( mCameraSceneUndistorted );
    ui->graphicsView_undistorted->setBackgroundBrush( QBrush( QColor(150,50,50) ) );

    // <<<<< Stream rendering

    mElabPool.setMaxThreadCount( 3 );
}

MainWindow::~MainWindow()
{
    killGstLaunch();

    while( mGstProcess.state() == QProcess::Running )
    {
        mGstProcess.kill();
        QApplication::processEvents( QEventLoop::AllEvents, 50 );
    }

    mElabPool.clear();

    delete ui;
    delete mCameraThread;
    delete mCameraSceneRaw;
    delete mCameraSceneCheckboard;
    delete mCameraSceneUndistorted;
    delete mCameraCalib;
}

QString MainWindow::updateOpenCvVer()
{
    QString ocvVers = tr("OpenCV %1.%2.%3").arg(CV_MAJOR_VERSION).arg(CV_MINOR_VERSION).arg(CV_SUBMINOR_VERSION);

    return ocvVers;
}

QStringList MainWindow::updateCameraInfo()
{
    QStringList res;

    mCameras = getCameraDescriptions();
    for (const auto& cameraInfo : mCameras)
    {
        res.push_back(cameraInfo.id);
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
    if( mCameras.empty() ) {
        return;
}

    if( index>mCameras.size()-1  ) {
        return;
}

    if( index<0 )
    {
        ui->label_camera->setText( tr("No camera info") );
    }
    else
    {
        ui->label_camera->setText(mCameras.at(index).description);

        ui->comboBox_camera_res->clear();

        for (const auto& mode : mCameras[index].modes)
        {
            ui->comboBox_camera_res->addItem(mode.getDescr());
        }

    }
}

bool MainWindow::startCamera()
{
    if(!killGstLaunch()) {
        return false;
    }

    if(!startGstProcess()) {
        return false;
    }

    delete mCameraThread;
    mCameraThread = nullptr;

    const auto& mode = mCameras[ui->comboBox_camera->currentIndex()].modes[ui->comboBox_camera_res->currentIndex()];

    mCameraThread = new CameraThread(mode.fps());

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
    killGstLaunch();

    if( mCameraThread )
    {
        disconnect( mCameraThread, &CameraThread::cameraConnected,
                    this, &MainWindow::onCameraConnected );
        disconnect( mCameraThread, &CameraThread::cameraDisconnected,
                    this, &MainWindow::onCameraDisconnected );
        disconnect( mCameraThread, &CameraThread::newImage,
                    this, &MainWindow::onNewImage );

        delete mCameraThread;
        mCameraThread = nullptr;
    }
}

void MainWindow::onCameraConnected()
{
    mCameraConnected = true;
}

void MainWindow::onCameraDisconnected()
{
    mCameraConnected = false;

    ui->pushButton_camera_connect_disconnect->setChecked(false);
    ui->pushButton_camera_connect_disconnect->setText( tr("Start Camera") );
    stopCamera();

    ui->lineEdit_cb_cols->setEnabled(true);
    ui->lineEdit_cb_rows->setEnabled(true);
    ui->lineEdit_cb_mm->setEnabled(true);
    ui->lineEdit_cb_max_count->setEnabled(true);

    ui->comboBox_camera->setEnabled(true);
    ui->comboBox_camera_res->setEnabled(true);

    ui->pushButton_load_params->setEnabled(true);
    ui->pushButton_save_params->setEnabled(false);

    QMessageBox::warning( this, tr("Camera error"), tr("Camera disconnected\n"
                                                       "If the camera has been just started\n"
                                                       "please verify the correctness of\n"
                                                       "Width, Height and FPS"));
}

void MainWindow::onNewImage( cv::Mat frame )
{
    static int frmCnt=0;
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

    frmCnt++;

    if( ui->pushButton_calibrate->isChecked() && frmCnt%((int)mSrcFps) == 0 )
    {
        auto* elab = new QChessboardElab( this, frame, mCbSize, mCbSizeMm, mCameraCalib );
        mElabPool.tryStart(elab);
    }

    cv::Mat rectified = mCameraCalib->undistort( frame );

    if( rectified.empty() )
    {
        mCameraSceneUndistorted->setFgImage(frame);
        ui->graphicsView_undistorted->setBackgroundBrush( QBrush( QColor(150,50,50) ) );
    }
    else
    {
        mCameraSceneUndistorted->setFgImage(rectified);
        ui->graphicsView_undistorted->setBackgroundBrush( QBrush( QColor(50,150,50) ) );
    }

    if(mCameraThread)
    {
        double perc = mCameraThread->getBufPerc();

        int percInt = static_cast<int>(perc*100);

        ui->progressBar_camBuffer->setValue(percInt);
    }
}

void MainWindow::onNewCbImage(cv::Mat cbImage)
{
    mCameraSceneCheckboard->setFgImage(cbImage);

    ui->lineEdit_cb_count->setText( tr("%1").arg(mCameraCalib->getCbCount()) );
}

void MainWindow::onCbDetected()
{
    //qDebug() << tr("Beep");

    mCbDetectedSnd->play();
}

void MainWindow::onNewCameraParams(cv::Mat K, cv::Mat D, bool refining, double calibReprojErr)
{
    if( refining )
    {
        mCalibInfo.setText( tr("Refining existing Camera parameters") );
    }
    else
    {
        mCalibInfo.setText( tr("Estimating new Camera parameters") );
    }

    ui->lineEdit_calib_reproj_err->setText(tr("%1").arg(calibReprojErr));

    if(calibReprojErr<=0.5 )
    {
        ui->lineEdit_calib_reproj_err->setStyleSheet("QLineEdit { background: rgb(50, 250, 50);}");
    }
    else if(calibReprojErr<=1.0 && calibReprojErr>0.5)
    {
        ui->lineEdit_calib_reproj_err->setStyleSheet("QLineEdit { background: rgb(250, 250, 50);}");
    }
    else
    {
        ui->lineEdit_calib_reproj_err->setStyleSheet("QLineEdit { background: rgb(250, 50, 50);}");
    }

    if( !K.empty() && !D.empty() )
    {
        updateParamGUI( K, D );
    }
}

void MainWindow::on_pushButton_camera_connect_disconnect_clicked(bool checked)
{
    if( checked )
    {
        const auto& mCamera = mCameras[ui->comboBox_camera->currentIndex()];
        const auto& mode = mCamera.modes[ui->comboBox_camera_res->currentIndex()];

        mLaunchLine = mCamera.launchLine;

        mSrcWidth = mode.w;
        mSrcHeight = mode.h;
        mSrcFps = mode.fps();
        mSrcFpsNum = mode.num;
        mSrcFpsDen = mode.den;

        updateCbParams();

        if(mCameraCalib)
        {
            disconnect( mCameraCalib, &QCameraCalibrate::newCameraParams,
                        this, &MainWindow::onNewCameraParams );

            delete mCameraCalib;
        }

        bool fisheye = ui->checkBox_fisheye->isChecked();

        mCameraCalib = new QCameraCalibrate( cv::Size(mSrcWidth, mSrcHeight), mCbSize, mCbSizeMm, fisheye );

        connect( mCameraCalib, &QCameraCalibrate::newCameraParams,
                 this, &MainWindow::onNewCameraParams );

        cv::Size imgSize;
        cv::Mat K;
        cv::Mat D;
        double alpha;
        mCameraCalib->getCameraParams( imgSize, K, D, alpha, fisheye );

        ui->checkBox_fisheye->setChecked(fisheye);
        ui->horizontalSlider_alpha->setValue( static_cast<int>( alpha*ui->horizontalSlider_alpha->maximum() ) );

        updateParamGUI(K,D);

        if( startCamera() )
        {
            ui->pushButton_camera_connect_disconnect->setText( tr("Stop Camera") );

            ui->lineEdit_cb_cols->setEnabled(false);
            ui->lineEdit_cb_rows->setEnabled(false);
            ui->lineEdit_cb_mm->setEnabled(false);
            ui->lineEdit_cb_max_count->setEnabled(false);

            ui->comboBox_camera->setEnabled(false);
            ui->comboBox_camera_res->setEnabled(false);

            ui->pushButton_load_params->setEnabled(false);
            ui->pushButton_save_params->setEnabled(true);
        }
        else
        {
            ui->pushButton_camera_connect_disconnect->setText( tr("Start Camera") );
            ui->pushButton_camera_connect_disconnect->setChecked(false);

            ui->lineEdit_cb_cols->setEnabled(true);
            ui->lineEdit_cb_rows->setEnabled(true);
            ui->lineEdit_cb_mm->setEnabled(true);
            ui->lineEdit_cb_max_count->setEnabled(true);

            ui->comboBox_camera->setEnabled(true);
            ui->comboBox_camera_res->setEnabled(true);

            ui->pushButton_load_params->setEnabled(true);
            ui->pushButton_save_params->setEnabled(false);
        }
    }
    else
    {
        ui->pushButton_camera_connect_disconnect->setText( tr("Start Camera") );
        stopCamera();

        ui->lineEdit_cb_cols->setEnabled(true);
        ui->lineEdit_cb_rows->setEnabled(true);
        ui->lineEdit_cb_mm->setEnabled(true);
        ui->lineEdit_cb_max_count->setEnabled(true);

        ui->comboBox_camera->setEnabled(true);
        ui->comboBox_camera_res->setEnabled(true);

        ui->pushButton_load_params->setEnabled(true);
        ui->pushButton_save_params->setEnabled(false);
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

bool MainWindow::killGstLaunch( )
{
    // >>>>> Kill gst-launch-1.0 processes

#ifdef Q_OS_WIN

    QProcess killer;
    killer.start("taskkill /im gst-launch-1.0.exe /f /t");
    killer.waitForFinished(1000);

#else

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

            return false;
        }

    }
    while( !done );
    // <<<<< Kill gst-launch-1.0 processes
#endif // Q_OS_WIN

    return true;
}

bool MainWindow::startGstProcess( )
{
    // handle command line analogously to https://github.com/GStreamer/gst-plugins-base/blob/master/tools/gst-device-monitor.c
    if(mCameras.empty()) {
        return false;
    }

    QString launchStr;

#ifdef USE_ARM
    launchStr = QStringLiteral(
                "gst-launch-1.0 %1 do-timestamp=true ! "
                "\"video/x-raw,format=I420,width=%2,height=%3,framerate=%4/%5\" ! nvvidconv ! "
                "\"video/x-raw(memory:NVMM),width=%2,height=%3\" ! "
                //"omxh264enc low-latency=true insert-sps-pps=true ! "
                "omxh264enc insert-sps-pps=true ! "
                "rtph264pay config-interval=1 pt=96 mtu=9000 ! queue ! "
                "udpsink host=127.0.0.1 port=5000 sync=false async=false -e"
                ).arg(mLaunchLine).arg(mSrcWidth).arg(mSrcHeight).arg(mSrcFpsDen).arg(mSrcFpsNum);
#elif defined(Q_OS_WIN)
    launchStr =
        QStringLiteral("gst-launch-1.0.exe %1 ! "
            "video/x-raw,format=I420,width=%2,height=%3,framerate=%4/%5 ! videoconvert ! "
            //"videoscale ! \"video/x-raw,width=%5,height=%6\" ! "
            "x264enc key-int-max=1 tune=zerolatency bitrate=8000 ! "
            "rtph264pay config-interval=1 pt=96 mtu=9000 ! queue ! "
            "udpsink host=127.0.0.1 port=5000 sync=false async=false -e")
        .arg(mLaunchLine).arg(mSrcWidth).arg(mSrcHeight).arg(mSrcFpsDen).arg(mSrcFpsNum);
#else
    launchStr =
        QStringLiteral("gst-launch-1.0 %1 ! "
               "\"video/x-raw,format=I420,width=%2,height=%3,framerate=%4/%5\" ! videoconvert ! "
               //"videoscale ! \"video/x-raw,width=%5,height=%6\" ! "
               "x264enc key-int-max=1 tune=zerolatency bitrate=8000 ! "
               "rtph264pay config-interval=1 pt=96 mtu=9000 ! queue ! "
               "udpsink host=127.0.0.1 port=5000 sync=false async=false -e")
            .arg(mLaunchLine).arg(mSrcWidth).arg(mSrcHeight).arg(mSrcFpsDen).arg(mSrcFpsNum);
#endif

    qDebug() << tr("Starting pipeline: \n %1").arg(launchStr);

    mGstProcess.setProcessChannelMode( QProcess::MergedChannels );

    connect(&mGstProcess, &QProcess::readyReadStandardOutput, [&pingProcess = this->mGstProcess]() {
        QString output = pingProcess.readAllStandardOutput();
        qDebug() << "Child process trace: " << output;
    });

    mGstProcess.start( launchStr );

    if( !mGstProcess.waitForStarted( 5000 ) )
    {
        // TODO Camera error message
        qDebug() << "Timed out starting a child process";
        return false;
    }

    return true;
}

void MainWindow::updateCbParams()
{
    mCbSize.width = ui->lineEdit_cb_cols->text().toInt();
    mCbSize.height = ui->lineEdit_cb_rows->text().toInt();
    mCbSizeMm = ui->lineEdit_cb_mm->text().toFloat();
}

void MainWindow::updateParamGUI( cv::Mat K, cv::Mat D )
{
    double fx = K.ptr<double>(0)[0];
    double fy = K.ptr<double>(1)[1];
    double cx = K.ptr<double>(0)[2];
    double cy = K.ptr<double>(1)[2];
    double scale = K.ptr<double>(2)[2];

    ui->lineEdit_fx->setText( tr("%1").arg(fx) );
    ui->lineEdit_fy->setText( tr("%1").arg(fy) );
    ui->lineEdit_cx->setText( tr("%1").arg(cx) );
    ui->lineEdit_cy->setText( tr("%1").arg(cy) );
    ui->lineEdit_scale->setText( tr("%1").arg(scale) );

    double k1 = D.ptr<double>(0)[0];
    double k2 = D.ptr<double>(1)[0];

    if( ui->checkBox_fisheye->isChecked() )
    {
        double k3 = D.ptr<double>(2)[0];
        double k4 = D.ptr<double>(3)[0];

        ui->lineEdit_k1->setText( tr("%1").arg(k1) );
        ui->lineEdit_k2->setText( tr("%1").arg(k2) );
        ui->lineEdit_k3->setText( tr("%1").arg(k3) );
        ui->lineEdit_k4->setText( tr("%1").arg(k4) );

        ui->lineEdit_k5->setVisible(false);
        ui->lineEdit_k6->setVisible(false);
        ui->lineEdit_p1->setVisible(false);
        ui->lineEdit_p2->setVisible(false);
    }
    else
    {
        double p1 = D.ptr<double>(2)[0];
        double p2 = D.ptr<double>(3)[0];

        double k3 = D.ptr<double>(4)[0];
        double k4 = D.ptr<double>(5)[0];
        double k5 = D.ptr<double>(6)[0];
        double k6 = D.ptr<double>(7)[0];

        ui->lineEdit_k1->setText( tr("%1").arg(k1) );
        ui->lineEdit_k2->setText( tr("%1").arg(k2) );
        ui->lineEdit_p1->setText( tr("%1").arg(p1) );
        ui->lineEdit_p2->setText( tr("%1").arg(p2) );
        ui->lineEdit_k3->setText( tr("%1").arg(k3) );
        ui->lineEdit_k4->setText( tr("%1").arg(k4) );
        ui->lineEdit_k5->setText( tr("%1").arg(k5) );
        ui->lineEdit_k6->setText( tr("%1").arg(k6) );

        ui->lineEdit_k5->setVisible(true);
        ui->lineEdit_k6->setVisible(true);
        ui->lineEdit_p1->setVisible(true);
        ui->lineEdit_p2->setVisible(true);
    }
}

void MainWindow::setNewCameraParams()
{
    if( !mCameraCalib )
    {
        return;
    }

    cv::Mat K(3, 3, CV_64F, cv::Scalar::all(0.0F) );
    cv::Mat D( 8, 1, CV_64F, cv::Scalar::all(0.0F) );

    K.ptr<double>(0)[0] = ui->lineEdit_fx->text().toDouble();
    K.ptr<double>(0)[1] = ui->lineEdit_K_01->text().toDouble();
    K.ptr<double>(0)[2] = ui->lineEdit_cx->text().toDouble();
    K.ptr<double>(1)[0] = ui->lineEdit_K_10->text().toDouble();
    K.ptr<double>(1)[1] = ui->lineEdit_fy->text().toDouble();
    K.ptr<double>(1)[2] = ui->lineEdit_cy->text().toDouble();
    K.ptr<double>(2)[0] = ui->lineEdit_K_20->text().toDouble();
    K.ptr<double>(2)[1] = ui->lineEdit_K_21->text().toDouble();
    K.ptr<double>(2)[2] = ui->lineEdit_scale->text().toDouble();

    D.ptr<double>(0)[0] = ui->lineEdit_k1->text().toDouble();
    D.ptr<double>(1)[0] = ui->lineEdit_k2->text().toDouble();

    if(ui->checkBox_fisheye->isChecked())
    {
        D.ptr<double>(2)[0] = ui->lineEdit_k3->text().toDouble();
        D.ptr<double>(3)[0] = ui->lineEdit_k4->text().toDouble();
        //D.ptr<double>(4)[0] = 0.0;
        //D.ptr<double>(5)[0] = 0.0;
        //D.ptr<double>(6)[0] = 0.0;
        //D.ptr<double>(7)[0] = 0.0;
    }
    else
    {
        D.ptr<double>(2)[0] = ui->lineEdit_p1->text().toDouble();
        D.ptr<double>(3)[0] = ui->lineEdit_p2->text().toDouble();
        D.ptr<double>(4)[0] = ui->lineEdit_k3->text().toDouble();
        D.ptr<double>(5)[0] = ui->lineEdit_k4->text().toDouble();
        D.ptr<double>(6)[0] = ui->lineEdit_k5->text().toDouble();
        D.ptr<double>(7)[0] = ui->lineEdit_k6->text().toDouble();
    }

    double alpha = static_cast<double>(ui->horizontalSlider_alpha->value())/ui->horizontalSlider_alpha->maximum();
    bool fisheye = ui->checkBox_fisheye->isChecked();

    mCameraCalib->setCameraParams( cv::Size(mSrcWidth,mSrcHeight), K, D, alpha, fisheye );
}

void MainWindow::on_lineEdit_fx_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_K_01_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_cx_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_K_10_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_fy_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_cy_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_K_20_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_K_21_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_scale_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k1_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k2_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k3_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k4_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k5_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_k6_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_p1_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_lineEdit_p2_editingFinished()
{
    setNewCameraParams();
}

void MainWindow::on_pushButton_calibrate_clicked(bool checked)
{
    ui->groupBox_params->setEnabled(!checked);
}

void MainWindow::on_pushButton_load_params_clicked()
{
    QString filter1 = tr("OpenCV YAML (*.yaml *.yml)");
    QString filter2 = tr("XML (*.xml)");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Save Camera Calibration Parameters"), QDir::homePath(),
                                                    tr("%1;;%2").arg(filter1).arg(filter2) );

    if( fileName.isEmpty() ) {
        return;
}

    // Not using the function from CameraUndistort to verify that they are coherent before setting them

    cv::FileStorage fs( fileName.toStdString(), cv::FileStorage::READ||cv::FileStorage::FORMAT_AUTO );

    if( fs.isOpened() )
    {
        int w;
        int h;
        bool fisheye;
        double alpha;

        fs["Width"] >> w;
        fs["Height"] >> h;
        fs["FishEye"] >> fisheye;
        fs["Alpha"] >> alpha;

        const auto& camera = mCameras[ui->comboBox_camera->currentIndex()];

        bool matched = false;
        for( int i = 0; i < camera.modes.size(); i++ )
        {
            const auto& mode = camera.modes[ui->comboBox_camera_res->currentIndex()];

            if( mode.w==w && mode.h==h )
            {
                matched=true;
                ui->comboBox_camera_res->setCurrentIndex(i);
                break;
            }
        }

        if(!matched)
        {
            QMessageBox::warning( this, tr("Warning"), tr("Current camera does not support the resolution\n"
                                                          "%1x%2 loaded from the file:\n"
                                                          "%3").arg(w).arg(h).arg(fileName));
            return;
        }

        cv::Mat K;
        cv::Mat D;

        fs["CameraMatrix"] >> K;
        fs["DistCoeffs"] >> D;

        ui->checkBox_fisheye->setChecked(fisheye);

        ui->horizontalSlider_alpha->setValue( static_cast<int>(alpha*ui->horizontalSlider_alpha->maximum()) );

        updateParamGUI(K,D);

        setNewCameraParams();
    }
}

void MainWindow::on_pushButton_save_params_clicked()
{
    if( !mCameraCalib ) {
        return;
}

    QString selFilter;

    QString filter1 = tr("OpenCV YAML (*.yaml *.yml)");
    QString filter2 = tr("XML (*.xml)");

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Camera Calibration Parameters"), QDir::homePath(),
                                                    tr("%1;;%2").arg(filter1).arg(filter2), &selFilter);

    if( fileName.isEmpty() ) {
        return;
}

    if( !fileName.endsWith( ".yaml", Qt::CaseInsensitive) &&
            !fileName.endsWith( ".yml", Qt::CaseInsensitive) &&
            !fileName.endsWith( ".xml", Qt::CaseInsensitive) )
    {
        if( selFilter == filter2 )
        {
            fileName += ".xml";
        }
        else
        {
            fileName += ".yaml";
        }
    }

    cv::FileStorage fs( fileName.toStdString(), cv::FileStorage::WRITE );

    if( fs.isOpened() )
    {
        cv::Size imgSize;
        cv::Mat K;
        cv::Mat D;
        bool fisheye;
        double alpha;

        mCameraCalib->getCameraParams( imgSize,K,D,alpha,fisheye );

        fs << "Width" << imgSize.width;
        fs << "Height" << imgSize.height;
        fs << "FishEye" << fisheye;
        fs << "Alpha" << alpha;
        fs << "CameraMatrix" << K;
        fs << "DistCoeffs" << D;
    }
}


void MainWindow::on_horizontalSlider_alpha_valueChanged(int value)
{
    if( mCameraCalib )
    {
        double alpha = (double)value/(ui->horizontalSlider_alpha->maximum());
        mCameraCalib->setNewAlpha(alpha);
    }
}

void MainWindow::on_checkBox_fisheye_clicked(bool checked)
{
    if( mCameraCalib )
    {
        mCameraCalib->setFisheye( checked );
    }
    cv::Size imgSize;
    cv::Mat K;
    cv::Mat D;
    bool fisheye;
    double alpha;

    mCameraCalib->getCameraParams( imgSize,K,D,alpha,fisheye );

    updateParamGUI( K,D );
}
