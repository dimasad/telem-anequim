#include "Gauge.hpp"

#include <QtGlobal>
#include <QGraphicsSvgItem>
#include <QGraphicsSimpleTextItem>
#include <QLocale>
#include <QDebug>

#include <algorithm>
#include <cmath>

typedef enum {
    AnchorTop, AnchorBottom, AnchorLeft, AnchorRight,
    AnchorTopLeft, AnchorTopRight, AnchorBottomLeft, AnchorBottomRight
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
    for (auto majorTick: m_majorTicks)
        delete majorTick;
    for (auto majorTickLabel: m_majorTickLabels)
        delete majorTickLabel;
    
    m_majorTicks.clear();
    m_majorTickLabels.clear();
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
        slideToPos(tick, pos);
        m_majorTicks.append(tick);
        
        QGraphicsSimpleTextItem *tickLabel = new QGraphicsSimpleTextItem();
        tickLabel->setText(QLocale().toString(value));
        tickLabel->setBrush(QColor("white"));
        tickLabel->setZValue(InfoLayer);
        scene()->addItem(tickLabel);
        m_majorTickLabels.append(tickLabel);

        if (i == 0)
            placeFirstTickLabel(tick, tickLabel);
        else if (i == m_numMajorTicks - 1)
            placeLastTickLabel(tick, tickLabel);
        else
            placeTickLabel(tick, tickLabel);
    }
}


void
LinearGauge::setValue(double value)
{
    slideToPos(&m_needle, valueToPos(value));
}


double
LinearGauge::valueToPos(double value)
{

    if (value < m_valueMin)
        return m_startPos;
    
    if (value > m_valueMax)
        return m_endPos;

    double normalizedValue = (value - m_valueMin) / (m_valueMax - m_valueMin);
    return normalizedValue * std::abs(m_endPos - m_startPos) +
        std::min(m_startPos, m_endPos);
}


HorizontalLinearGauge::HorizontalLinearGauge(QWidget *parent)
    : LinearGauge(parent)
{    
    m_renderer.load(QString(":/images/linear-horizontal-gauge.svg"));
    
    m_startPos = m_renderer.boundsOnElement("firstTick").center().x();
    m_endPos = m_renderer.boundsOnElement("lastTick").center().x();
    
    initializeFromId(&m_background, "background", BackgroundLayer);
    initializeFromId(&m_needle, "needle", NeedleLayer);
}


void
HorizontalLinearGauge::slideToPos(QGraphicsItem *item, double pos)
{
    item->setX(pos - item->boundingRect().width() / 2);
}


void
HorizontalLinearGauge::placeFirstTickLabel(QGraphicsItem *tick, 
                                           QGraphicsItem *label)
{
    anchorItem(label, AnchorTopLeft, tick->sceneBoundingRect().bottomLeft());
}


void
HorizontalLinearGauge::placeTickLabel(QGraphicsItem *tick,
                                      QGraphicsItem *label)
{
    anchorItem(label, AnchorTop, bottomCenter(tick->sceneBoundingRect()));    
}


void
HorizontalLinearGauge::placeLastTickLabel(QGraphicsItem *tick, 
                                          QGraphicsItem *label)
{
    anchorItem(label, AnchorTopRight, tick->sceneBoundingRect().bottomRight());
}


VerticalLinearGauge::VerticalLinearGauge(QWidget *parent)
    : LinearGauge(parent)
{    
    m_renderer.load(QString(":/images/linear-vertical-gauge.svg"));
    
    m_startPos = m_renderer.boundsOnElement("firstTick").center().y();
    m_endPos = m_renderer.boundsOnElement("lastTick").center().y();
    
    initializeFromId(&m_background, "background", BackgroundLayer);
    initializeFromId(&m_needle, "needle", NeedleLayer);
}


void
VerticalLinearGauge::slideToPos(QGraphicsItem *item, double pos)
{
    item->setY(pos - item->boundingRect().height() / 2);
}


void
VerticalLinearGauge::placeFirstTickLabel(QGraphicsItem *tick, 
                                           QGraphicsItem *label)
{
    anchorItem(label, AnchorTopRight, tick->sceneBoundingRect().bottomRight());
}


void
VerticalLinearGauge::placeTickLabel(QGraphicsItem *tick,
                                      QGraphicsItem *label)
{
    anchorItem(label, AnchorTopRight, tick->sceneBoundingRect().bottomRight());
}


void
VerticalLinearGauge::placeLastTickLabel(QGraphicsItem *tick, 
                                          QGraphicsItem *label)
{
    anchorItem(label, AnchorBottomRight, tick->sceneBoundingRect().topRight());
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
    case AnchorBottomLeft:
        item->setX(mappedPosition.x());
        item->setY(mappedPosition.y() - sceneBoundingRect.height());
        break;
    case AnchorBottomRight:
        item->setX(mappedPosition.x() - sceneBoundingRect.width());
        item->setY(mappedPosition.y() - sceneBoundingRect.height());
        break;
    }
}


static QPointF
bottomCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.bottom());
}


SvgGauge::SvgGauge(const QString &svgFile, QWidget *parent) :
    QGraphicsView(parent), m_renderer(svgFile)
{
    setScene(new QGraphicsScene(this));
    setStyleSheet("background: transparent");
    
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setSceneRect(m_renderer.viewBoxF());
}


QGraphicsSvgItem*
SvgGauge::addItemFromElement(const QString &elementId, qreal zValue)
{
    QGraphicsSvgItem *element = new QGraphicsSvgItem;
    element->setSharedRenderer(&m_renderer);
    element->setElementId(elementId);
    element->setPos(m_renderer.boundsOnElement(elementId).topLeft());
    element->setZValue(zValue);
    
    scene()->addItem(element);
    
    return element;
}


void
SvgGauge::resizeEvent(QResizeEvent *event)
{
    fitInView(sceneRect(), Qt::KeepAspectRatio);
    QGraphicsView::resizeEvent(event);
}


void
TickedSvgGauge::setNumMajorTicks(unsigned newNumMajorTicks)
{
    m_numMajorTicks = newNumMajorTicks;
    updateMajorTicks();
}


void
TickedSvgGauge::setTextColor(const QColor &newColor)
{
    m_textColor = newColor;
    for (auto tickLabel: m_majorTickLabels)
        tickLabel->setBrush(m_textColor);
    for (auto textLabel: m_textLabels)
        textLabel->setBrush(m_textColor);
    
}


void
TickedSvgGauge::setValueRange(double valueMin, double valueMax)
{
    Q_ASSERT(valueMin <= valueMax);
    
    m_valueMin = valueMin;
    m_valueMax = valueMax;
    
    updateMajorTicks();
}


void
TickedSvgGauge::updateMajorTicks()
{
    for (auto majorTick: m_majorTicks)
        delete majorTick;
    for (auto majorTickLabel: m_majorTickLabels)
        delete majorTickLabel;
    
    m_majorTicks.clear();
    m_majorTickLabels.clear();
    
    double valueRange = m_valueMax - m_valueMin;
    double valueIncrement = valueRange / std::max(m_numMajorTicks - 1, 1u);
    for (unsigned i = 0; i < m_numMajorTicks; i++)
        placeMajorTick(m_valueMin + i * valueIncrement);
}


AngularSvgGauge::AngularSvgGauge(const QString &svgFile, QWidget *parent) :
    TickedSvgGauge(svgFile, parent)
{
    m_pivot = m_renderer.boundsOnElement("pivot").center();
    
    m_background = addItemFromElement("background", BackgroundLayer);
    m_needle = addItemFromElement("needle", NeedleLayer);
    m_foreground = addItemFromElement("foreground", ForegroundLayer);    
}


void
AngularSvgGauge::setAngleRange(double angleMin, double angleMax)
{
    Q_ASSERT(angleMin <= angleMax);

    m_angleMin = angleMin;    
    m_angleMax = angleMax;
}


void
AngularSvgGauge::setValue(double value)
{
    m_needle->setRotation(valueToAngle(value));
    if (m_valueLabel != 0)
        m_valueLabel->setText(QLocale().toString(value));
}


void
AngularSvgGauge::setValueLabelPos(double xPos, double yPos)
{
    if (m_valueLabel == 0) {
        m_valueLabel = new QGraphicsSimpleTextItem;
        m_valueLabel->setBrush(m_textColor);
        m_valueLabel->setZValue(InfoLayer);
        m_textLabels.append(m_valueLabel);
    }
    
    m_valueLabel->setPos(xPos, yPos);
    scene()->addItem(m_valueLabel);
}


QGraphicsSvgItem*
AngularSvgGauge::addItemFromElement(const QString &elementId, qreal zValue)
{
    auto element = TickedSvgGauge::addItemFromElement(elementId, zValue);
    element->setTransformOriginPoint(element->mapFromScene(m_pivot));
    
    return element;
}


void 
AngularSvgGauge::placeMajorTick(double value)
{
    double angle = valueToAngle(value);

    auto tick = addItemFromElement("majorTick", InfoLayer);
    tick->setRotation(angle);
    m_majorTicks.append(tick);
    
    auto *tickLabel = new QGraphicsSimpleTextItem();
    tickLabel->setText(QLocale().toString(value));
    tickLabel->setBrush(m_textColor);
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


double
AngularSvgGauge::valueToAngle(double value)
{
    if (value < m_valueMin)
        return m_angleMin;
    
    if (value > m_valueMax)
        return m_angleMax;
    
    double normalizedValue = (value - m_valueMin) / (m_valueMax - m_valueMin);
    double angle = normalizedValue * (m_angleMax - m_angleMin) + m_angleMin;
    
    return remainder(angle, 360);
}
