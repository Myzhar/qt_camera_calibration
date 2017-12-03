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

class QCameraUndistort;

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
    void onNewCameraParams(cv::Mat K, cv::Mat D, bool refining);

protected slots:
    void onCameraConnected();
    void onCameraDisconnected();
    void onProcessReadyRead();

    void updateParamGUI();

private slots:
    void on_pushButton_update_camera_list_clicked();
    void on_comboBox_camera_currentIndexChanged(int index);
    void on_pushButton_camera_connect_disconnect_clicked(bool checked);
    void on_pushButton_load_params_clicked();
    void on_pushButton_save_params_clicked();
    void on_lineEdit_chessboard_cols_editingFinished();
    void on_lineEdit__chessboard_rows_editingFinished();
    void on_lineEdit__chessboard_mm_editingFinished();

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
    int mSrcFps;

    cv::Size mCbSize;
    float mCbSizeMm;

    QThreadPool mElabPool;

    QCameraUndistort* mCameraUndist;
};

#endif // MAINWINDOW_H
