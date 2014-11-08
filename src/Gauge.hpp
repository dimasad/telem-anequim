#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QSharedPointer>
#include <QSvgRenderer>

/* TODO:
 * - Make rangeBands in Gauges.
 * - Make minor ticks.
 */


class SvgGauge : public QGraphicsView
{
    Q_OBJECT

public:
    SvgGauge(const QString &svgFile, QWidget *parent=0);

protected:
    QSvgRenderer m_renderer;
    
    QGraphicsSvgItem* addItemFromElement(const QString &elementId, 
                                         qreal zValue);
    void resizeEvent(QResizeEvent *event);
};


class TickedSvgGauge : public SvgGauge
{
    Q_OBJECT
    
public:
    using SvgGauge::SvgGauge;
    void setNumMajorTicks(unsigned newNumMajorTicks);
    void setTextColor(const QColor &newColor);
    void setValueRange(double valueMin, double valueMax);
    
protected:
    double m_valueMin = 0, m_valueMax = 1;
    unsigned m_numMajorTicks = 0;
    QColor m_textColor = QColor("black");
    QList<QGraphicsSvgItem *> m_majorTicks;
    QList<QGraphicsSimpleTextItem *> m_majorTickLabels;
    QList<QGraphicsSimpleTextItem *> m_textLabels;
    
    void updateMajorTicks();
    
    virtual void placeMajorTick(double value) = 0;
};


class AngularSvgGauge : public TickedSvgGauge
{
    Q_OBJECT
    
public:
    AngularSvgGauge(const QString &svgFile, QWidget *parent=0);
    void addLabel(const QString &text, double x, double y);
    void addRangeBand(const QColor &color, double startValue, double endValue);
    void setAngleRange(double angleMin, double angleMax);
    void setValue(double value);
    void setValueLabelPos(double xPos, double yPos);
    
protected:
    enum GraphicLayers {
        BackgroundLayer, RangeBandLayer, InfoLayer, NeedleLayer, ForegroundLayer
    };
    
    QGraphicsSvgItem *m_background, *m_needle, *m_foreground;
    QGraphicsSimpleTextItem *m_valueLabel = 0;
    QPointF m_pivot;
    double m_angleMin = -90, m_angleMax = 90;
    double m_rangeBandInnerRadius, m_rangeBandOuterRadius;

    QGraphicsSvgItem* addItemFromElement(const QString &elementId, 
                                         qreal zValue);
    void placeMajorTick(double value);
    double valueToAngle(double value);    
};


class LinearSvgGauge : public TickedSvgGauge
{
    Q_OBJECT
    
public:
    LinearSvgGauge(const QString &svgFile, QWidget *parent=0);
    void addRangeBand(const QColor &color, double startValue, double endValue);
    void setValue(double value);
    
protected:
    enum GraphicLayers {
        BackgroundLayer, InfoLayer, CursorLayer, ForegroundLayer
    };
    
    QGraphicsSvgItem *m_background, *m_cursor, *m_foreground;
    QGraphicsSimpleTextItem *m_valueLabel = 0;
    QRectF m_cursorRange;
    double m_startPos, m_endPos;
    
    void moveToPos(QGraphicsItem *item, double pos);
    void placeMajorTick(double value);
    double valueToPos(double value);
};


#endif // GAUGE_HPP
