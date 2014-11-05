#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QSharedPointer>
#include <QSvgRenderer>

/* TODO:
 * - Make rangeBands in Gauges.
 * - Make minor ticks.
 * - Make loading of gauges from JSON.
 * - Fix Gauge widget scaling and minimum and preferred widths and heights.
 */


class Gauge : public QGraphicsView
{
    Q_OBJECT
    
public:
    Gauge(QWidget *parent=0);
    void setValueLimits(double min, double max);
    virtual void setNumMajorTicks(unsigned numMajorTicks) = 0;
    unsigned numMajorTicks() {return m_numMajorTicks;}
    void addLabel(const QString &text, double xPos, double yPos);
    void setValueLabelPos(double xPos, double yPos);
    void clearTicks();

public slots:
    virtual void setValue(double value) = 0;

protected:
    enum GraphicLayers {
        BackgroundLayer, InfoLayer, NeedleLayer, ForegroundLayer
    };
    
    double m_valueMin = 0, m_valueMax = 100;
    unsigned m_numMajorTicks;
    QSvgRenderer m_renderer;
    QGraphicsSvgItem m_background;
    QGraphicsSvgItem m_needle;
    QGraphicsSvgItem m_foreground;
    QList<QGraphicsSvgItem *> m_majorTicks;
    QList<QGraphicsSimpleTextItem *> m_majorTickLabels;
    QGraphicsSimpleTextItem m_valueLabel;
    
    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);
};


class AngularGauge : public Gauge
{
    Q_OBJECT
    
public:
    AngularGauge(QWidget *parent=0);
    void setAngleLimits(double min, double max);
    virtual void setNumMajorTicks(unsigned numMajorTicks);
    
public slots:
    virtual void setValue(double value);
    
protected:
    double m_angleMin = -135, m_angleMax = 90;
    QPointF m_pivot;
    
    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);    
    double valueToAngle(double value);
};


class LinearGauge : public Gauge
{
    Q_OBJECT
    
public:
    LinearGauge(QWidget *parent=0) : Gauge(parent) {}
    virtual void setNumMajorTicks(unsigned numMajorTicks);
    
public slots:
    virtual void setValue(double value);
    
protected:    
    double m_startPos, m_endPos;

    double valueToPos(double value);
    virtual void slideToPos(QGraphicsItem *item, double pos) = 0;
    virtual void placeFirstTickLabel(QGraphicsItem *tick, 
                                     QGraphicsItem *label) = 0;
    virtual void placeTickLabel(QGraphicsItem *tick,
                                QGraphicsItem *label) = 0;
    virtual void placeLastTickLabel(QGraphicsItem *tick, 
                                    QGraphicsItem *label) = 0;
};


class HorizontalLinearGauge : public LinearGauge
{
    Q_OBJECT
    
public:
    HorizontalLinearGauge(QWidget *parent=0);

protected:
    virtual void slideToPos(QGraphicsItem *item, double pos);
    virtual void placeFirstTickLabel(QGraphicsItem *tick, 
                                     QGraphicsItem *label);
    virtual void placeTickLabel(QGraphicsItem *tick,
                                QGraphicsItem *label);
    virtual void placeLastTickLabel(QGraphicsItem *tick, 
                                    QGraphicsItem *label);
};


class VerticalLinearGauge : public LinearGauge
{
    Q_OBJECT
    
public:
    VerticalLinearGauge(QWidget *parent=0);
    
protected:
    virtual void slideToPos(QGraphicsItem *item, double pos);
    virtual void placeFirstTickLabel(QGraphicsItem *tick,
                                     QGraphicsItem *label);
    virtual void placeTickLabel(QGraphicsItem *tick,
                                QGraphicsItem *label);
    virtual void placeLastTickLabel(QGraphicsItem *tick,
                                    QGraphicsItem *label);
};


class SvgGauge : public QGraphicsView
{
    Q_OBJECT

public:
    SvgGauge(const QString &svgFile, QWidget *parent=0);

protected:
    QSvgRenderer m_renderer;
    
    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);
    void resizeEvent(QResizeEvent *event);
};


class TickedSvgGauge : public SvgGauge
{
    Q_OBJECT
    
public:
    TickedSvgGauge(const QString &svgFile, QWidget *parent=0);
    void setNumMajorTicks(unsigned newNumMajorTicks);
    void setValueRange(double valueMin, double valueMax);
    
protected:
    double m_valueMin = 0, m_valueMax = 1;
    unsigned m_numMajorTicks = 0;
    QList<QGraphicsSvgItem *> m_majorTicks;
    QList<QGraphicsSimpleTextItem *> m_majorTickLabels;
    
    void updateMajorTicks();
    
    virtual void placeMajorTick(double value) = 0;
};


class AngularSvgGauge : public TickedSvgGauge
{
    Q_OBJECT
    
public:
    AngularSvgGauge(const QString &svgFile, QWidget *parent=0);
    void setAngleRange(double angleMin, double angleMax);
    void setValue(double value);
    void setValueLabelPos(double xPos, double yPos);
    
protected:
    enum GraphicLayers {
        BackgroundLayer, InfoLayer, NeedleLayer, ForegroundLayer
    };
    
    QGraphicsSvgItem m_background, m_needle, m_foreground;
    QGraphicsSimpleTextItem m_valueLabel;
    QPointF m_pivot;
    double m_angleMin = -90, m_angleMax = 90;

    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);
    void placeMajorTick(double value);
    double valueToAngle(double value);    
};


#endif // GAUGE_HPP
