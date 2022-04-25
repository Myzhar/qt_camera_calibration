#include "include/v4l2compcamera.h"

#include <cstdio>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <sys/ioctl.h>
#endif // Q_OS_LINUX

#ifdef Q_OS_WINDOWS

#include <windows.h>
#include <dshow.h>

#include <atlbase.h>

#pragma comment(lib, "strmiids")

#endif


#include <QString>
#include <QDebug>

V4L2CompCamera::V4L2CompCamera(int w, int h, int fpsNum, int fpsDen)
    : QObject(nullptr)
{
    mWidth = w;
    mHeight = h;
    mFPS = (double)fpsDen/fpsNum;

    mFpsNum = fpsNum;
    mFpsDen = fpsDen;

    mDescr = tr("%1 x %2 @ %3 FPS [ %4 / %5 sec ]").arg(mWidth).arg(mHeight).arg(mFPS).arg(mFpsNum).arg(mFpsDen);
}

V4L2CompCamera::V4L2CompCamera(const V4L2CompCamera& other)
    : QObject(nullptr)
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

/*
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
*/

std::string fcc2s(unsigned int val)
{
    std::string s;

    s += val & 0x7f;
    s += (val >> 8) & 0x7f;
    s += (val >> 16) & 0x7f;
    s += (val >> 24) & 0x7f;
    if (val & (1 << 31)) {
        s += "-BE";
    }
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
    return ok;
}

#ifdef Q_OS_LINUX

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
#endif // Q_OS_LINUX

#ifdef Q_OS_WINDOWS

namespace {

class CComUsageScope
{
    bool m_bInitialized;
public:
    explicit CComUsageScope(DWORD dwCoInit = COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY)
    {
        m_bInitialized = SUCCEEDED(CoInitializeEx(nullptr, dwCoInit));
    }
    ~CComUsageScope()
    {
        if (m_bInitialized) {
            CoUninitialize();
}
    }
};

HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = nullptr;
    }
    if (mt.pUnk != nullptr)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = nullptr;
    }
}


// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != nullptr)
    {
        _FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}


} // namespace

QList<V4L2CompCamera> V4L2CompCamera::enumCompFormats(QString dev)
{
    CComUsageScope scope;

    CComPtr<IEnumMoniker> pEnum;

    if (FAILED(EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum))) {
        return {};
    }

    QList<V4L2CompCamera> result;

    IMoniker *pMoniker{};

    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
    {
        CComPtr<IPropertyBag> pPropBag;
        HRESULT hr = pMoniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }

        VARIANT var;
        VariantInit(&var);

        bool selected = false;

        hr = pPropBag->Read(L"DevicePath", &var, nullptr);
        if (SUCCEEDED(hr))
        {
            QString v((const QChar*)var.bstrVal);

            selected = dev.contains(v);

            VariantClear(&var);
        }

        CComPtr<IBaseFilter> ppDevice;

        //we get the filter
        if (selected && SUCCEEDED(pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&ppDevice)))
        {
            CComPtr<IEnumPins> pEnumPins;
            if (SUCCEEDED((ppDevice->EnumPins(&pEnumPins))))
            {
                IPin *pPin = nullptr;
                while (pEnumPins->Next(1, &pPin, nullptr) == S_OK)
                {
                    PIN_DIRECTION direction;
                    if (SUCCEEDED(pPin->QueryDirection(&direction))
                        && direction == PINDIR_OUTPUT)
                    //PIN_INFO info;
                    //if (SUCCEEDED(pPin->QueryPinInfo(&info))
                    //    && info.dir == PINDIR_OUTPUT)
                    {
                        CComPtr<IEnumMediaTypes> pEnum;
                        AM_MEDIA_TYPE *pmt = nullptr;

                        if (SUCCEEDED(pPin->EnumMediaTypes(&pEnum)))
                        {
                            while (pEnum->Next(1, &pmt, nullptr) == S_OK)
                            {
                                if ((pmt->formattype == FORMAT_VideoInfo) &&
                                    (pmt->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                                    (pmt->pbFormat != nullptr))
                                {
                                    auto videoInfoHeader = (VIDEOINFOHEADER*)pmt->pbFormat;

                                    if (videoInfoHeader->AvgTimePerFrame < 10000000)
                                    {
                                        V4L2CompCamera camFmt(videoInfoHeader->bmiHeader.biWidth,
                                            videoInfoHeader->bmiHeader.biHeight,
                                            videoInfoHeader->AvgTimePerFrame,
                                            10000000);

                                        result << camFmt;
                                    }
                                }

                                _DeleteMediaType(pmt);
                            }
                        }
                    }
                    if (pPin) {
                        pPin->Release();
                    }
                }
            }
        }

        pMoniker->Release();
    }

    return result;
}

#endif
