#include "qopencvscene.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QList>

#include <opencv2/highgui/highgui.hpp>

QOpenCVScene::QOpenCVScene(QObject *parent) :
    QGraphicsScene(parent),
    mBgPixmapItem(nullptr),
    mTrackRect(nullptr)
{
    setBackgroundBrush( QBrush(QColor(255,255,255)));

    mTrackRect = new QGraphicsRectItem(0.0,0.0,0.0,0.0);

    QPen borderPen;
    borderPen.setWidth( 2 );
    borderPen.setCapStyle(Qt::RoundCap);
    borderPen.setColor( Qt::darkYellow );
    mTrackRect->setPen( borderPen );

    addItem( mTrackRect );
    mTrackRect->setZValue( 2000.0 );
}

QOpenCVScene::~QOpenCVScene()
{
    delete mBgPixmapItem;
}

void QOpenCVScene::setFgImage( cv::Mat& cvImg )
{
    if(!mBgPixmapItem)
    {
        mBgPixmapItem = new QGraphicsPixmapItem( cvMatToQPixmap(cvImg) );
        //cv::imshow( "Test", cvImg );
        mBgPixmapItem->setPos( 0,0 );

        addItem( mBgPixmapItem );
    }
    else {
        mBgPixmapItem->setPixmap( cvMatToQPixmap(cvImg) );
}

    //cv::imshow( "Test", cvImg );
    //qDebug() << tr("Image: %1 x %2").arg(cvImg.cols).arg(cvImg.rows);

    mBgPixmapItem->setZValue( -10.0 );
    setSceneRect( 0,0, cvImg.cols, cvImg.rows );
    update();
}

void QOpenCVScene::setFgImage( QImage& img)
{
    if(!mBgPixmapItem)
    {
        QPixmap pmap;
        pmap.convertFromImage( img );
        mBgPixmapItem = new QGraphicsPixmapItem( pmap );

        mBgPixmapItem->setPos( 0,0 );

        addItem( mBgPixmapItem );
    }
    else
    {
        QPixmap pmap;
        pmap.convertFromImage( img );
        mBgPixmapItem->setPixmap( pmap );
    }

    mBgPixmapItem->setZValue( 1000.0 );
    setSceneRect( 0,0, img.width(), img.height() );
}

QImage QOpenCVScene::cvMatToQImage( const cv::Mat &inMat )
{
    switch ( inMat.type() )
    {
    // 8-bit, 4 channel
    case CV_8UC4:
    {
        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32 );

        return image;
    }

        // 8-bit, 3 channel
    case CV_8UC3:
    {
        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB888 );

        return image.rgbSwapped();
    }

        // 8-bit, 1 channel
    case CV_8UC1:
    {
        static QVector<QRgb>  sColorTable;

        // only create our color table once
        if ( sColorTable.isEmpty() )
        {
            for ( int i = 0; i < 256; ++i ) {
                sColorTable.push_back( qRgb( i, i, i ) );
}
        }

        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_Indexed8 );

        image.setColorTable( sColorTable );

        return image;
    }

    default:
        qWarning() << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
        break;
    }

    return {};
}

QPixmap QOpenCVScene::cvMatToQPixmap( const cv::Mat &inMat )
{
    return QPixmap::fromImage( cvMatToQImage( inMat ) );
}

//void QOpenCVScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
//{
//    if( event->button() == Qt::LeftButton )
//    {
//        QPointF mouseScenePos = event->buttonDownScenePos( Qt::MouseButton::LeftButton );

//        QRectF scRect = sceneRect();

//        //qreal normX = (mouseScenePos.x()/scRect.width());
//        //qreal normY = (mouseScenePos.y()/scRect.height());

//        mTrackRect->setVisible( true );

//        QRectF rect = QRectF(mouseScenePos,mouseScenePos).normalized();
//        mTrackRect->setRect( rect );

//        //emit newLeftMousePress( normX, normY );
//    }
//}

//void QOpenCVScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
//{
//    if( event->buttons() == Qt::LeftButton )
//    {
//        static int count =0;
//        count++;
//        QPointF mouseDwScenePos = event->buttonDownScenePos( Qt::MouseButton::LeftButton );
//        QPointF mousePos = event->scenePos();

//        QRectF rect = QRectF(mouseDwScenePos,mousePos).normalized();

//        /*qDebug() << "*********************** " << count;
//        qDebug() << mouseDwScenePos;
//        qDebug() << mousePos;
//        qDebug() << rect;*/

//        mTrackRect->setRect( rect );
//    }
//}

//void QOpenCVScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
//{
//    if( event->button() == Qt::LeftButton )
//    {
//        /*QPointF mouseDwScenePos = event->buttonDownScenePos( Qt::MouseButton::LeftButton );
//        QPointF mouseUpScenePos = event->scenePos();*/

//        QRectF rect = mTrackRect->rect();

//        QRectF scRect = sceneRect();

//        qreal normX = (rect.topLeft().x()/scRect.width());
//        qreal normY = (rect.topLeft().y()/scRect.height());

//        qreal normW = (rect.width()/scRect.width());
//        qreal normH = (rect.height()/scRect.height());

//        if( normX < 0.02 )
//            normX = 0.02;
//        if( normY < 0.02 )
//            normY = 0.02;
//        if( normX > 0.98 )
//            normX = 0.98;
//        if( normY > 0.98 )
//            normY = 0.98;

//        if( normX+normW>0.98 )
//            normW = 0.98-normX;
//        if( normY+normH>0.95 )
//            normH = 0.98-normY;

//        emit mouseClicked( normX, normY, normW, normH, 0 );

//        mTrackRect->setRect( 0.0,0.0,0.0,0.0 );

//        mTrackRect->setVisible( false );
//    }
//    else
//    {
//        emit mouseClicked( 0, 0, 0, 0, 1 );
//    }
//}
