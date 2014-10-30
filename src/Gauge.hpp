#ifndef GAUGE_HPP
#define GAUGE_HPP

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QSharedPointer>
#include <QSvgRenderer>

/* TODO:
 * - Make AngularGauge and LinearGauge.
 * - Make rangeBands in Gauges.
 * - Make minor ticks.
 * - Make loading of gauges from JSON.
 * - Add labels to gauges.
 * - Add digital display of value.
 * - Fix Gauge background
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
    
    virtual void initializeFromId(QGraphicsSvgItem *element,
                                  const QString &elementId, qreal zValue);
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
    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);
    
private:    
    double m_angleMin = -135, m_angleMax = 90;
    QPointF m_pivot;
    
    double valueToAngle(double value);
};

#endif // GAUGE_HPP
