#ifndef QOPENCVSCENE_H
#define QOPENCVSCENE_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QImage>
#include <QPixmap>

#include <opencv2/core/core.hpp>

class QOpenCVScene : public QGraphicsScene
{
    Q_OBJECT
public:
    /// Default constructor
    explicit QOpenCVScene(QObject *parent = 0);
    virtual ~QOpenCVScene();

public slots:
    /// Sets Background Image from OpenCV cv::Mat
    void setFgImage(const cv::Mat& cvImg);
    void setFgImage(const QPixmap& img);
    void setFgImage(const QImage& img);

//    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
    /// Converts cv::Mat to QImage
    static QImage  cvMatToQImage( const cv::Mat &inMat );
    /// Converts cv::Mat to QPixmap
    static QPixmap cvMatToQPixmap( const cv::Mat &inMat );

signals:
    void mouseClicked( qreal normX, qreal normY, qreal normW, qreal normH, quint8 button);

private:    
    QGraphicsPixmapItem* mBgPixmapItem; ///< Background image

    QGraphicsRectItem* mTrackRect; ///< Tracking rectangle
};


#endif // QOPENCVSCENE_H
