#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCameraInfo>
#include <QProcess>
#include <QThreadPool>
#include <QSound>

#include <opencv2/core/core.hpp>

class CameraThread;
class QOpenCVScene;

class QCameraUndistort;

namespace Ui
{
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
    void onCbDetected();
    void onNewCameraParams(cv::Mat K, cv::Mat D, bool refining, double calibReprojErr );

protected slots:
    void onCameraConnected();
    void onCameraDisconnected();
    void onProcessReadyRead();

    void updateParamGUI();
    void updateCbParams();
    void setNewCameraParams();

private slots:
    void on_pushButton_update_camera_list_clicked();
    void on_comboBox_camera_currentIndexChanged(int index);
    void on_pushButton_camera_connect_disconnect_clicked(bool checked);

    void on_lineEdit_fx_editingFinished();
    void on_lineEdit_K_01_editingFinished();
    void on_lineEdit_cx_editingFinished();
    void on_lineEdit_K_10_editingFinished();
    void on_lineEdit_fy_editingFinished();
    void on_lineEdit_cy_editingFinished();
    void on_lineEdit_K_20_editingFinished();
    void on_lineEdit_K_21_editingFinished();
    void on_lineEdit_scale_editingFinished();
    void on_lineEdit_k1_editingFinished();
    void on_lineEdit_k2_editingFinished();
    void on_lineEdit_k3_editingFinished();
    void on_lineEdit_k4_editingFinished();
    void on_lineEdit_k5_editingFinished();
    void on_lineEdit_k6_editingFinished();
    void on_lineEdit_p1_editingFinished();
    void on_lineEdit_p2_editingFinished();

    void on_pushButton_calibrate_clicked(bool checked);

    void on_pushButton_load_params_clicked();
    void on_pushButton_save_params_clicked();

    void on_checkBox_fisheye_clicked();

    void on_comboBox_camera_currentIndexChanged(const QString &arg1);

    void on_horizontalSlider_alpha_valueChanged(int value);

private:
    Ui::MainWindow *ui;

    QLabel mOpenCvVer;
    QLabel mCalibInfo;

    QProcess mGstProcess;

    QList<QCameraInfo> mCameras;
    CameraThread* mCameraThread;
    bool mCameraConnected;

    QOpenCVScene* mCameraSceneRaw;
    QOpenCVScene* mCameraSceneCheckboard;
    QOpenCVScene* mCameraSceneUndistorted;

    cv::Mat mLastFrame;
    cv::Mat mIntrinsic;
    cv::Mat mDistorsion;

    QString mCamDev;
    int mSrcWidth;
    int mSrcHeight;
    double mSrcFps;

    cv::Size mCbSize;
    float mCbSizeMm;

    QThreadPool mElabPool;

    QCameraUndistort* mCameraUndist;

    QSound* mCbDetectedSnd;
};

#endif // MAINWINDOW_H
