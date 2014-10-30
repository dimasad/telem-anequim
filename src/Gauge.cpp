#include "Gauge.hpp"

#include <QGraphicsSvgItem>
#include <QGraphicsSimpleTextItem>
#include <QLocale>
#include <QDebug>

#include <algorithm>
#include <cmath>

typedef enum {
    AnchorTop, AnchorBottom, AnchorLeft, AnchorRight,
    AnchorTopLeft, AnchorTopRight
} AnchorPoint;

static QPointF bottomCenter(const QRectF &rect);
static void anchorItem(QGraphicsItem *item, AnchorPoint anchor, 
                       const QPointF &position);



Gauge::Gauge(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setStyleSheet("background: transparent");

    m_valueLabel.setBrush(QColor("white"));
    m_valueLabel.setZValue(InfoLayer);
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


void
Gauge::addLabel(const QString &text, double xPos, double yPos)
{
    QGraphicsSimpleTextItem *label = new QGraphicsSimpleTextItem(text);
    label->setPos(xPos, yPos);
    label->setBrush(QColor("white"));
    label->setZValue(InfoLayer);
    scene()->addItem(label);
}


void
Gauge::setValueLabelPos(double xPos, double yPos)
{
    m_valueLabel.setPos(xPos, yPos);
    scene()->addItem(&m_valueLabel);
}


void
Gauge::clearTicks()
{
    for (auto majorTick : m_majorTicks)
        delete majorTick;
    for (auto majorTickLabel : m_majorTickLabels)
        delete majorTickLabel;
    
    m_majorTicks.clear();
    m_majorTickLabels.clear();
}


AngularGauge::AngularGauge(QWidget *parent)
    : Gauge(parent)
{    
    m_renderer.load(QString(":/images/angular-gauge.svg"));
    m_pivot = m_renderer.boundsOnElement("pivot").center();
    
    initializeFromId(&m_background, "background", BackgroundLayer);
    initializeFromId(&m_needle, "needle", NeedleLayer);
    initializeFromId(&m_foreground, "foreground", ForegroundLayer);    
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
    m_valueLabel.setText(QLocale().toString(value));
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
    clearTicks();
    
    double valueRange = m_valueMax - m_valueMin;
    double valueIncrement = valueRange / std::max(m_numMajorTicks - 1, 1u);
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
        tickLabel->setZValue(InfoLayer);
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


LinearGauge::LinearGauge(QWidget *parent)
    : Gauge(parent)
{    
    m_renderer.load(QString(":/images/linear-gauge.svg"));
    m_startPos = m_renderer.boundsOnElement("firstTick").center().x();
    m_endPos = m_renderer.boundsOnElement("lastTick").center().x();
    m_cursorWidth = m_renderer.boundsOnElement("needle").width();
    m_tickWidth = m_renderer.boundsOnElement("firstTick").width();
    
    initializeFromId(&m_background, "background", BackgroundLayer);
    initializeFromId(&m_needle, "needle", NeedleLayer);
}

void
LinearGauge::setNumMajorTicks(unsigned numMajorTicks)
{
    m_numMajorTicks = numMajorTicks;
    clearTicks();
    
    double valueRange = m_valueMax - m_valueMin;
    double valueIncrement = valueRange / std::max(m_numMajorTicks - 1, 1u);
    for (unsigned i = 0; i < m_numMajorTicks; i++) {
        double value = m_valueMin + i * valueIncrement;
        double pos = valueToPos(value);
        
        QGraphicsSvgItem *tick = new QGraphicsSvgItem;
        initializeFromId(tick, "firstTick", InfoLayer);
        tick->setX(pos - m_tickWidth / 2);
        m_majorTicks.append(tick);
        
        QGraphicsSimpleTextItem *tickLabel = new QGraphicsSimpleTextItem();
        tickLabel->setText(QLocale().toString(value));
        tickLabel->setBrush(QColor("white"));
        tickLabel->setZValue(InfoLayer);
        scene()->addItem(tickLabel);
        m_majorTickLabels.append(tickLabel);

        if (i == 0) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomLeft();
            anchorItem(tickLabel, AnchorTopLeft, anchorPoint);
        } else if (i == m_numMajorTicks - 1) {
            QPointF anchorPoint = tick->sceneBoundingRect().bottomRight();
            anchorItem(tickLabel, AnchorTopRight, anchorPoint);
        } else {
            QPointF anchorPoint = bottomCenter(tick->sceneBoundingRect());
            anchorItem(tickLabel, AnchorTop, anchorPoint);
        }

    }
}


void
LinearGauge::setValue(double value)
{
    m_needle.setX(valueToPos(value) - m_cursorWidth / 2);
}


double
LinearGauge::valueToPos(double value)
{

    if (value < m_valueMin)
        return m_startPos;
    
    if (value > m_valueMax)
        return m_endPos;

    double normalizedValue = (value - m_valueMin) / (m_valueMax - m_valueMin);
    return normalizedValue * (m_endPos - m_startPos) + m_startPos;
}


static void
anchorItem(QGraphicsItem *item, AnchorPoint anchor, 
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
    case AnchorTopLeft:
        item->setX(mappedPosition.x());
        item->setY(mappedPosition.y());
        break;
    case AnchorTopRight:
        item->setX(mappedPosition.x() - sceneBoundingRect.width());
        item->setY(mappedPosition.y());
        break;
    }
}


static QPointF
bottomCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.bottom());
}
