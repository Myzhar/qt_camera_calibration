#include "mainwindow.h"
#include <QApplication>

#include <gst/gst.h>

int main(int argc, char *argv[])
{
    gst_init( &argc, &argv );

    QApplication a(argc, argv);

    QApplication::setApplicationName("qt_camera_calibration");
    QApplication::setOrganizationName("Myzhar");
    QApplication::setOrganizationDomain("www.myzhar.com");

    MainWindow w;
    w.showMaximized();

    return QApplication::exec();
}
