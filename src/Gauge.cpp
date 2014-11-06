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


SvgGauge::SvgGauge(const QString &svgFile, QWidget *parent) :
    QGraphicsView(parent), m_renderer(svgFile)
{
    setScene(new QGraphicsScene(this));
    setStyleSheet("background: transparent");
    
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setSceneRect(m_renderer.viewBoxF());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
AngularSvgGauge::addRangeBand(const QColor &color, 
                              double startValue, double endValue)
{
    Q_ASSERT(startValue <= endValue);
    
    double startAngle = -valueToAngle(startValue) - 90;
    double endAngle = -valueToAngle(endValue) - 90;
    
    double outerRadius = (m_renderer.boundsOnElement("minorTick").top() - 
                          m_pivot.y());
    double innerRadius = (m_renderer.boundsOnElement("minorTick").bottom() - 
                          m_pivot.y());
    QRectF outerRect(m_pivot.x() - outerRadius, m_pivot.y() - outerRadius,
                     2 * outerRadius, 2 * outerRadius);
    QRectF innerRect(m_pivot.x() - innerRadius, m_pivot.y() - innerRadius,
                     2 * innerRadius, 2 * innerRadius);
    
    QPainterPath path(m_pivot);
    path.arcTo(outerRect, startAngle, endAngle - startAngle);
    path.arcTo(innerRect, endAngle, -(endAngle - startAngle));
    
    auto band = new QGraphicsPathItem(path);
    band->setZValue(RangeBandLayer);
    band->setBrush(color);
    band->setPen(QPen(Qt::NoPen));
    scene()->addItem(band);
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


LinearSvgGauge::LinearSvgGauge(const QString &svgFile, QWidget *parent) :
    TickedSvgGauge(svgFile, parent)
{
    m_cursorRange = m_renderer.boundsOnElement("cursorRange");
    m_startPos = m_cursorRange.left();
    m_endPos = m_cursorRange.right();
    
    m_background = addItemFromElement("background", BackgroundLayer);
    m_cursor = addItemFromElement("cursor", CursorLayer);
    m_foreground = addItemFromElement("foreground", ForegroundLayer);
}


void
LinearSvgGauge::addRangeBand(const QColor &color, 
                             double startValue, double endValue)
{
    Q_ASSERT(startValue <= endValue);

    QRectF bandRect(m_cursorRange);
    bandRect.setLeft(valueToPos(startValue));
    bandRect.setRight(valueToPos(endValue));
    
    auto band = new QGraphicsRectItem(bandRect);
    band->setZValue(BackgroundLayer);
    band->setBrush(color);
    band->setPen(QPen(Qt::NoPen));
    scene()->addItem(band);
}


void
LinearSvgGauge::setValue(double value)
{
    moveToPos(m_cursor, valueToPos(value));
}


void
LinearSvgGauge::placeMajorTick(double value)
{
    double pos = valueToPos(value);
    auto tick = addItemFromElement("majorTick", InfoLayer);
    moveToPos(tick, pos);
    m_majorTicks.append(tick);

    QGraphicsSimpleTextItem *tickLabel = new QGraphicsSimpleTextItem();
    tickLabel->setText(QLocale().toString(value));
    tickLabel->setBrush(QColor("white"));
    tickLabel->setZValue(InfoLayer);
    scene()->addItem(tickLabel);
    m_majorTickLabels.append(tickLabel);
    
    QRectF tickBoundingRect = tick->sceneBoundingRect();
    if (value == m_valueMin)
        anchorItem(tickLabel, AnchorTopLeft, tickBoundingRect.bottomLeft());
    else if (value == m_valueMax)
        anchorItem(tickLabel, AnchorTopRight, tickBoundingRect.bottomRight());
    else
        anchorItem(tickLabel, AnchorTop, bottomCenter(tickBoundingRect));
}


double
LinearSvgGauge::valueToPos(double value)
{
    if (value < m_valueMin)
        return m_startPos;
    
    if (value > m_valueMax)
        return m_endPos;

    double normalizedValue = (value - m_valueMin) / (m_valueMax - m_valueMin);
    return (normalizedValue * std::abs(m_endPos - m_startPos) +
            std::min(m_startPos, m_endPos));
}


void
LinearSvgGauge::moveToPos(QGraphicsItem *item, double pos)
{
    item->setX(pos - item->sceneBoundingRect().width() / 2);
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
