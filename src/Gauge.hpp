#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QSharedPointer>
#include <QSvgRenderer>


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
    void setNumMinorTicks(unsigned newNumMinorTicks);
    void setTextColor(const QColor &newColor);
    void setValueLabelFormat(char f, int precision);
    void setValueRange(double valueMin, double valueMax);
    
protected:
    double m_valueMin = 0, m_valueMax = 1;
    unsigned m_numMajorTicks = 0, m_numMinorTicks = 0;
    int m_valueLabelPrecision = 8;
    char m_valueLabelFormat = 'g';
    QColor m_textColor = QColor("black");
    QList<QGraphicsSvgItem *> m_majorTicks;
    QList<QGraphicsSvgItem *> m_minorTicks;
    QList<QGraphicsSimpleTextItem *> m_majorTickLabels;
    QList<QGraphicsSimpleTextItem *> m_textLabels;
    QGraphicsSimpleTextItem *m_valueLabel = 0;
    
    void updateMajorTicks();
    void updateMinorTicks();
    
    virtual void placeMajorTick(double value) = 0;
    virtual void placeMinorTick(double value) = 0;
};


class AngularSvgGauge : public TickedSvgGauge
{
    Q_OBJECT
    
public:
    AngularSvgGauge(const QString &svgFile, QWidget *parent=0);
    void addRangeBand(const QColor &color, double startValue, double endValue);
    void setAngleRange(double angleMin, double angleMax);
    void setValue(double value);
    void setBottomLabel(const QString &text);
    void setTopLabel(const QString &text);
    
protected:
    enum GraphicLayers {
        BackgroundLayer, RangeBandLayer, InfoLayer, NeedleLayer, ForegroundLayer
    };
    
    QGraphicsSvgItem *m_background, *m_needle, *m_foreground;
    QPointF m_pivot, m_valueLabelCenter;
    double m_angleMin = -90, m_angleMax = 90;
    double m_rangeBandInnerRadius, m_rangeBandOuterRadius;

    QGraphicsSvgItem* addItemFromElement(const QString &elementId, 
                                         qreal zValue);
    QGraphicsSimpleTextItem* addLabelFromElement(const QString &text,
                                                 const QString &elementId);
    void placeMajorTick(double value);
    void placeMinorTick(double value);
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
    QRectF m_cursorRange, m_valueLabelRect;
    double m_startPos, m_endPos;
    
    void moveToPos(QGraphicsItem *item, double pos);
    void placeMajorTick(double value);
    void placeMinorTick(double value);
    double valueToPos(double value);
};


#endif // GAUGE_HPP
