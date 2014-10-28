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
 * - Make loading of Gauges from JSON.
 * - Fix Gauge background
 * - Fix Gauge widget scaling and minimum and preferred widths and heights.
 */

class Gauge : public QGraphicsView
{
    Q_OBJECT
    
public:
    Gauge(QWidget *parent=0);
    void setValueLimits(double min, double max);
    void setAngleLimits(double min, double max);
    
public slots:
    void setValue(double value);
    
protected:
    void initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                          qreal zValue);
    
private:
    enum GraphicLayers {
        BackgroundLayer, InfoLayer, NeedleLayer, ForegroundLayer
    };
    
    double m_valueMin = 0, m_valueMax = 100, m_angleMin = 20, m_angleMax = 340;
    unsigned m_numMajorTicks = 5;
    QPointF m_pivot;
    QSvgRenderer m_renderer;
    QGraphicsSvgItem m_background;
    QGraphicsSvgItem m_needle;
    QGraphicsSvgItem m_foreground;
    QList<QGraphicsSvgItem *> m_majorTicks;
    QList<QGraphicsSimpleTextItem *> m_majorTickLabels;
    
    double valueToAngle(double value);
    void setupMajorTicks();
};

#endif // GAUGE_HPP
