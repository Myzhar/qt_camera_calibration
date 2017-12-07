#include "include/v4l2compcamera.h"

#include <stdio.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <sys/ioctl.h>

#include <QString>
#include <QDebug>

V4L2CompCamera::V4L2CompCamera(int w, int h, int fpsNum, int fpsDen)
    : QObject(NULL)
{
    mWidth = w;
    mHeight = h;
    mFPS = (double)fpsDen/fpsNum;

    mFpsNum = fpsNum;
    mFpsDen = fpsDen;

    mDescr = tr("%1 x %2 @ %3 FPS [ %4 / %5 sec ]").arg(mWidth).arg(mHeight).arg(mFPS).arg(mFpsNum).arg(mFpsDen);
}

V4L2CompCamera::V4L2CompCamera(const V4L2CompCamera& other)
    : QObject(NULL)
{
    this->mWidth = other.mWidth;
    this->mHeight = other.mHeight;
    this->mFPS = other.mFPS;

    this->mFpsNum = other.mFpsNum;
    this->mFpsDen = other.mFpsDen;

    this->mDescr = other.mDescr;
}

V4L2CompCamera& V4L2CompCamera::operator=(const V4L2CompCamera& other)
{
    this->mWidth = other.mWidth;
    this->mHeight = other.mHeight;
    this->mFPS = other.mFPS;

    this->mFpsNum = other.mFpsNum;
    this->mFpsDen = other.mFpsDen;

    this->mDescr = other.mDescr;

    return *this;
}

static std::string num2s(unsigned num)
{
    char buf[10];

    sprintf(buf, "%08x", num);
    return buf;
}

std::string buftype2s(int type)
{
    switch (type)
    {
    case 0:
        return "Invalid";
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        return "Video Capture";
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        return "Video Capture Multiplanar";
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        return "Video Output";
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        return "Video Output Multiplanar";
    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        return "Video Overlay";
    case V4L2_BUF_TYPE_VBI_CAPTURE:
        return "VBI Capture";
    case V4L2_BUF_TYPE_VBI_OUTPUT:
        return "VBI Output";
    case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
        return "Sliced VBI Capture";
    case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
        return "Sliced VBI Output";
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
        return "Video Output Overlay";
    case V4L2_BUF_TYPE_SDR_CAPTURE:
        return "SDR Capture";
    case V4L2_BUF_TYPE_SDR_OUTPUT:
        return "SDR Output";
    default:
        return "Unknown (" + num2s(type) + ")";
    }
}

std::string fcc2s(unsigned int val)
{
    std::string s;

    s += val & 0x7f;
    s += (val >> 8) & 0x7f;
    s += (val >> 16) & 0x7f;
    s += (val >> 24) & 0x7f;
    if (val & (1 << 31))
        s += "-BE";
    return s;
}

bool V4L2CompCamera::descr2params( QString descr, int& width, int& height, double& fps, int& fpsNum, int& fpsDen )
{
    QStringList parts = descr.split( " " );

    if( parts.size() != 12 )
    {
        return false;
    }

    bool ok;

    width = static_cast<QString>(parts.at(0)).toInt( &ok );
    if( !ok )
    {
        return false;
    }

    height = static_cast<QString>(parts.at(2)).toInt( &ok );
    if( !ok )
    {
        return false;
    }

    fps = static_cast<QString>(parts.at(4)).toDouble( &ok );
    if( !ok )
    {
        return false;
    }

    fpsNum = static_cast<QString>(parts.at(7)).toDouble( &ok );
    if( !ok )
    {
        return false;
    }

    fpsDen = static_cast<QString>(parts.at(9)).toDouble( &ok );
    if( !ok )
    {
        return false;
    }

    return true;
}

QList<V4L2CompCamera> V4L2CompCamera::enumCompFormats( QString dev )
{
    QList<V4L2CompCamera> result;

    // >>>>> Camera formats

    v4l2_fmtdesc fmt;
    v4l2_frmsizeenum frmsize;
    v4l2_frmivalenum frmival;

    int fd = -1;

    const char* device = dev.toStdString().c_str();

    fd = open( device, O_RDWR);
    if (fd < 0)
    {
        qDebug() << tr( "Failed to open %1: %2")
                 .arg(device).arg(strerror(errno));
        return result;
    }

    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while( ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0)
    {
        if( fmt.type != V4L2_BUF_TYPE_VIDEO_CAPTURE )
        {
            fmt.index++;
            continue;
        }

        QString pxFormat = tr( "%1").arg( fcc2s(fmt.pixelformat).c_str() );

        if(pxFormat != "MJPG")
        {
            fmt.index++;
            continue;
        }

        frmsize.pixel_format = fmt.pixelformat;
        frmsize.index = 0;

        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0)
        {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                frmival.index = 0;
                frmival.pixel_format = fmt.pixelformat;
                frmival.width = frmsize.discrete.width;
                frmival.height = frmsize.discrete.height;

                while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0)
                {
                    frmival.index++;

                    V4L2CompCamera camFmt( frmival.width, frmival.height,
                                          (int)frmival.discrete.numerator, (int)frmival.discrete.denominator );

                    result << camFmt;

                    //qDebug() << camFmt.mDescr;
                }
            }
            frmsize.index++;
        }

        fmt.index++;
    }
    // <<<<< Camera formats

    return result;
}

