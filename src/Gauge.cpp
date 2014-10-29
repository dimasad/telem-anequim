#include "Gauge.hpp"

#include <QGraphicsSvgItem>
#include <QGraphicsSimpleTextItem>
#include <QLocale>
#include <QDebug>

#include <cmath>

typedef enum {AnchorTop, AnchorBottom, AnchorLeft, AnchorRight} AnchorPoint;


static void anchorItem(QGraphicsItem *item, AnchorPoint anchor, 
                       const QPointF &position);


Gauge::Gauge(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setStyleSheet("background: transparent");
}
    
void
Gauge::initializeFromId(QGraphicsSvgItem *element, const QString &elementId,
                        qreal zValue)
{
    element->setSharedRenderer(&m_renderer);
    element->setElementId(elementId);
    element->setPos(m_renderer.boundsOnElement(elementId).topLeft());
    element->setZValue(zValue);
    
    scene()->addItem(element);
}

void
Gauge::setValueLimits(double min, double max)
{
    m_valueMax = max;
    m_valueMin = min;
}

AngularGauge::AngularGauge(QWidget *parent)
    : Gauge(parent)
{    
    m_renderer.load(QString(":/images/gauge.svg"));
    m_pivot = m_renderer.boundsOnElement("pivot").center();
    
    initializeFromId(&m_background, "background", BackgroundLayer);
    initializeFromId(&m_needle, "needle", NeedleLayer);
    initializeFromId(&m_foreground, "foreground", ForegroundLayer);
    
    setNumMajorTicks(6);
}

void
AngularGauge::setAngleLimits(double min, double max)
{
    m_angleMax = max;
    m_angleMin = min;
}

void
AngularGauge::setValue(double value)
{
    m_needle.setRotation(valueToAngle(value));
}

void
AngularGauge::initializeFromId(QGraphicsSvgItem *element,
                               const QString &elementId, qreal zValue)
{
    Gauge::initializeFromId(element, elementId, zValue);
    element->setTransformOriginPoint(element->mapFromScene(m_pivot));
}

double
AngularGauge::valueToAngle(double value)
{
    if (value < m_valueMin)
        return m_angleMin;
    
    if (value > m_valueMax)
        return m_angleMax;
    
    double normalizedValue = (value - m_valueMin) / (m_valueMax - m_valueMin);
    double angle = normalizedValue * (m_angleMax - m_angleMin) + m_angleMin;
    
    return remainder(angle, 360);
}

void
AngularGauge::setNumMajorTicks(unsigned numMajorTicks)
{
    m_numMajorTicks = numMajorTicks;
    
    //Clear any existing old ticks
    while (!m_majorTicks.isEmpty()) {
        delete m_majorTicks.first();
        m_majorTicks.removeFirst();
    }
    
    if (m_numMajorTicks < 2)
        return;
    
    double valueIncrement = (m_valueMax - m_valueMin) / (m_numMajorTicks - 1);
    for (unsigned i = 0; i < m_numMajorTicks; i++) {
        double value = m_valueMin + i * valueIncrement;
        double angle = valueToAngle(value);
        
        QGraphicsSvgItem *tick = new QGraphicsSvgItem;
        initializeFromId(tick, "majorTick", InfoLayer);
        tick->setRotation(angle);
        m_majorTicks.append(tick);
        
        QGraphicsSimpleTextItem *tickLabel = new QGraphicsSimpleTextItem();
        tickLabel->setText(QLocale().toString(value));
        tickLabel->setBrush(QColor("white"));
        scene()->addItem(tickLabel);        
        m_majorTickLabels.append(tickLabel);

        if (angle < -135) {
            QPointF anchorPoint = tick->sceneBoundingRect().topRight();
            anchorItem(tickLabel, AnchorBottom, anchorPoint);
        } else if (angle >= -135 && angle < -90) {
            QPointF anchorPoint = tick->sceneBoundingRect().topRight();
            anchorItem(tickLabel, AnchorLeft, anchorPoint);
        } else if (angle >= -90 && angle < -45) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomRight();
            anchorItem(tickLabel, AnchorLeft, anchorPoint);
        } else if (angle >= -45 && angle < 0) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomRight();
            anchorItem(tickLabel, AnchorTop, anchorPoint);
        } else if (angle >= 0 && angle < 45) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomLeft();
            anchorItem(tickLabel, AnchorTop, anchorPoint);
        } else if (angle >= 45 && angle < 90) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomLeft();
            anchorItem(tickLabel, AnchorRight, anchorPoint);
        } else if (angle >= 90 && angle < 135) {
            QPointF anchorPoint = tick->sceneBoundingRect().topLeft();
            anchorItem(tickLabel, AnchorRight, anchorPoint);
        } else if (angle >= 135) {
            QPointF anchorPoint = tick->sceneBoundingRect().topLeft();
            anchorItem(tickLabel, AnchorBottom, anchorPoint);
        }
    }
}

static void anchorItem(QGraphicsItem *item, AnchorPoint anchor, 
                       const QPointF &scenePosition)
{
    QPointF mappedPosition = item->mapFromScene(scenePosition);
    QRectF sceneBoundingRect = item->sceneBoundingRect();

    switch (anchor) {
    case AnchorTop:
        item->setX(mappedPosition.x() - sceneBoundingRect.width() / 2);
        item->setY(mappedPosition.y());
        break;
    case AnchorBottom:
        item->setX(mappedPosition.x() - sceneBoundingRect.width() / 2);
        item->setY(mappedPosition.y() - sceneBoundingRect.height());
        break;
    case AnchorLeft:
        item->setX(mappedPosition.x());
        item->setY(mappedPosition.y() - sceneBoundingRect.height() / 2);
        break;
    case AnchorRight:
        item->setX(mappedPosition.x() - sceneBoundingRect.width());
        item->setY(mappedPosition.y() - sceneBoundingRect.height() / 2);
        break;
    }
}
