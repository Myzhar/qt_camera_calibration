#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCameraInfo>
#include <QProcess>
#include <QThreadPool>

#include <opencv2/core/core.hpp>

class CameraThread;
class QOpenCVScene;

class QFisheyeUndistort;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    QString updateOpenCvVer();
    QStringList updateCameraInfo();
    bool startGstProcess();
    bool killGstLaunch();
    bool startCamera();
    void stopCamera();

public slots:
    void onNewImage(cv::Mat frame);
    void onNewCbImage(cv::Mat cbImage);

protected slots:
    void onCameraConnected();
    void onCameraDisconnected();

    void onProcessReadyRead();

private slots:
    void on_pushButton_update_camera_list_clicked();
    void on_comboBox_camera_currentIndexChanged(int index);

    void on_pushButton_camera_connect_disconnect_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    QLabel mOpenCvVer;

    QProcess mGstProcess;

    QList<QCameraInfo> mCameras;
    CameraThread* mCameraThread;
    bool mCameraConnected;

    QOpenCVScene* mCameraSceneRaw;
    QOpenCVScene* mCameraSceneCheckboard;
    QOpenCVScene* mCameraSceneUndistorted;

    cv::Mat mLastFrame;

    QString mCamDev;
    int mSrcWidth;
    int mSrcHeight;
    int mSrcFps;

    cv::Size mCbSize;
    float mCbSizeMm;

    QThreadPool mElabPool;

    QFisheyeUndistort* mFisheyeUndist;
};

#endif // MAINWINDOW_H
