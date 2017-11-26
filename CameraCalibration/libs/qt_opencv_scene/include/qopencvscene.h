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
    void setFgImage( cv::Mat& cvImg );
    void setFgImage( QImage& img);

//    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    /// Converts cv::Mat to QImage
    QImage  cvMatToQImage( const cv::Mat &inMat );
    /// Converts cv::Mat to QPixmap
    QPixmap cvMatToQPixmap( const cv::Mat &inMat );

signals:
    void mouseClicked( qreal normX, qreal normY, qreal normW, qreal normH, quint8 button);

private:    
    QGraphicsPixmapItem* mBgPixmapItem; ///< Background image

    QGraphicsRectItem* mTrackRect; ///< Tracking rectangle
};


#endif // QOPENCVSCENE_H
