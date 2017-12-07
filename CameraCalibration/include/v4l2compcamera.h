#ifndef V4L2COMPCAMERA_H
#define V4L2COMPCAMERA_H

#include <QObject>
#include <QList>

// Enumerates all the available formats for a v4l2 device I420

class V4L2CompCamera : public QObject
{
    Q_OBJECT

public:
    V4L2CompCamera( int w, int h, double fps );
    V4L2CompCamera(const V4L2CompCamera& other);

    V4L2CompCamera& operator=(const V4L2CompCamera& other);

    static QList<V4L2CompCamera> enumCompFormats(QString dev );

    static bool descr2params( QString descr, int& width, int& height, double& fps );

    QString getDescr( )
    {
        return mDescr;
    }

private:
    int mWidth;
    int mHeight;
    double mFPS;

    QString mDescr;
};

#endif // V4L2COMPCAMERA_H()
