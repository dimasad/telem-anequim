#include "Gauge.hpp"

#include <QtGlobal>
#include <QFont>
#include <QGraphicsSvgItem>
#include <QGraphicsSimpleTextItem>
#include <QLocale>
#include <QDebug>

#include <algorithm>
#include <cmath>

typedef enum {
    AnchorTop, AnchorBottom, AnchorLeft, AnchorRight,
    AnchorTopLeft, AnchorTopRight, AnchorBottomLeft, AnchorBottomRight,
    AnchorCenter
} AnchorPoint;

static QPointF bottomCenter(const QRectF &rect);
static void anchorItem(QGraphicsItem *item, AnchorPoint anchor, 
                       const QPointF &anchorPoint);


SvgGauge::SvgGauge(const QString &svgFile, QWidget *parent) :
    QGraphicsView(parent), m_renderer(svgFile)
{
    setScene(new QGraphicsScene(this));
    setStyleSheet("background: transparent");
    
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setSceneRect(m_renderer.viewBoxF());
    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
TickedSvgGauge::setNumMinorTicks(unsigned newNumMinorTicks)
{
    m_numMinorTicks = newNumMinorTicks;
    updateMinorTicks();
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
TickedSvgGauge::setValueLabelFormat(char f, int precision)
{
    m_valueLabelFormat = f;
    m_valueLabelPrecision = precision;
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

    void updateMinorTicks();
}


void
TickedSvgGauge::updateMinorTicks()
{
    for (auto minorTick: m_minorTicks)
        delete minorTick;
    m_majorTicks.clear();

    if (m_numMinorTicks == 0 || m_numMajorTicks < 2)
        return;

    double majorRange = m_valueMax - m_valueMin;
    double majorIncrement = majorRange / std::max(m_numMajorTicks - 1, 1u);
    double minorIncrement = majorIncrement / (m_numMinorTicks + 1);
    
    for (unsigned i = 0; i < m_numMajorTicks; i++)
        for (unsigned j = 0; j < m_numMinorTicks; j++)
            placeMinorTick(m_valueMin + i * majorIncrement + 
                           (j + 1) * minorIncrement);
}


AngularSvgGauge::AngularSvgGauge(const QString &svgFile, QWidget *parent) :
    TickedSvgGauge(svgFile, parent)
{
    m_pivot = m_renderer.boundsOnElement("pivot").center();
    auto rangeBandRect = m_renderer.boundsOnElement("rangeBand");
    m_rangeBandInnerRadius = rangeBandRect.bottom() - m_pivot.y();
    m_rangeBandOuterRadius = rangeBandRect.top() - m_pivot.y();
    
    m_background = addItemFromElement("background", BackgroundLayer);
    m_needle = addItemFromElement("needle", NeedleLayer);
    m_foreground = addItemFromElement("foreground", ForegroundLayer);
    
    m_valueLabel = addLabelFromElement("", "valueLabel");
    m_valueLabelCenter = m_renderer.boundsOnElement("valueLabel").center();
}


void
AngularSvgGauge::addRangeBand(const QColor &color, 
                              double startValue, double endValue)
{
    Q_ASSERT(startValue <= endValue);
    
    double startAngle = -valueToAngle(startValue) - 90;
    double endAngle = -valueToAngle(endValue) - 90;
    QRectF innerRect(m_pivot.x() - m_rangeBandInnerRadius,
                     m_pivot.y() - m_rangeBandInnerRadius,
                     2 * m_rangeBandInnerRadius, 2 * m_rangeBandInnerRadius);
    QRectF outerRect(m_pivot.x() - m_rangeBandOuterRadius,
                     m_pivot.y() - m_rangeBandOuterRadius,
                     2 * m_rangeBandOuterRadius, 2 * m_rangeBandOuterRadius);
    
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
    if (m_valueLabel) {
        m_valueLabel->setText(QLocale().toString(value, m_valueLabelFormat,
                                                 m_valueLabelPrecision));
        anchorItem(m_valueLabel, AnchorCenter, m_valueLabelCenter);
    }
}


void
AngularSvgGauge::setBottomLabel(const QString &text)
{
    addLabelFromElement(text, "bottomLabel");
}


void
AngularSvgGauge::setTopLabel(const QString &text)
{
    addLabelFromElement(text, "topLabel");
}


QGraphicsSvgItem*
AngularSvgGauge::addItemFromElement(const QString &elementId, qreal zValue)
{
    auto element = TickedSvgGauge::addItemFromElement(elementId, zValue);
    element->setTransformOriginPoint(element->mapFromScene(m_pivot));
    
    return element;
}


QGraphicsSimpleTextItem*
AngularSvgGauge::addLabelFromElement(const QString &text, 
                                     const QString &elementId)
{
    QRectF labelRect = m_renderer.boundsOnElement(elementId);
    if (labelRect.isEmpty())
        return 0;
    
    QFont labelFont;
    labelFont.setPixelSize(labelRect.height());
    
    auto label = new QGraphicsSimpleTextItem(text);
    label->setZValue(InfoLayer);
    label->setBrush(m_textColor);
    label->setFont(labelFont);
    anchorItem(label, AnchorCenter, labelRect.center());
    scene()->addItem(label);
    m_textLabels.append(label);

    return label;
}


void 
AngularSvgGauge::placeMajorTick(double value)
{
    double angle = valueToAngle(value);

    auto tick = addItemFromElement("majorTick", InfoLayer);
    tick->setRotation(angle);
    m_majorTicks.append(tick);
    
    QRectF tickLabelRect = m_renderer.boundsOnElement("tickLabel");
    QFont tickLabelFont;
    tickLabelFont.setPixelSize(tickLabelRect.height());
    
    auto *tickLabel = new QGraphicsSimpleTextItem();
    tickLabel->setText(QLocale().toString(value));
    tickLabel->setFont(tickLabelFont);
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


void 
AngularSvgGauge::placeMinorTick(double value)
{
    double angle = valueToAngle(value);

    auto tick = addItemFromElement("minorTick", InfoLayer);
    tick->setRotation(angle);
    m_minorTicks.append(tick);
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

    m_valueLabelRect = m_renderer.boundsOnElement("valueLabel");
    m_valueLabel = new QGraphicsSimpleTextItem();
    m_valueLabel->setZValue(InfoLayer);
    m_valueLabel->setBrush(m_textColor);
    scene()->addItem(m_valueLabel);
    m_textLabels.append(m_valueLabel);

    QFont valueLabelFont;
    valueLabelFont.setPixelSize(m_valueLabelRect.height());
    m_valueLabel->setFont(valueLabelFont);
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
    m_valueLabel->setText(QLocale().toString(value, m_valueLabelFormat, 
                                             m_valueLabelPrecision));
    anchorItem(m_valueLabel, AnchorBottomRight, m_valueLabelRect.bottomRight());
}


void
LinearSvgGauge::placeMajorTick(double value)
{
    double pos = valueToPos(value);
    auto tick = addItemFromElement("majorTick", InfoLayer);
    moveToPos(tick, pos);
    m_majorTicks.append(tick);

    QRectF tickLabelRect = m_renderer.boundsOnElement("tickLabel");
    QFont tickLabelFont;
    tickLabelFont.setPixelSize(tickLabelRect.height());
    
    QGraphicsSimpleTextItem *tickLabel = new QGraphicsSimpleTextItem();
    tickLabel->setFont(tickLabelFont);
    tickLabel->setText(QLocale().toString(value));
    tickLabel->setBrush(QColor(m_textColor));
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


void
LinearSvgGauge::placeMinorTick(double value)
{
    double pos = valueToPos(value);
    auto tick = addItemFromElement("minorTick", InfoLayer);
    moveToPos(tick, pos);
    m_minorTicks.append(tick);
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
           const QPointF &anchorPoint)
{
    QRectF sceneBoundingRect = item->sceneBoundingRect();
    
    switch (anchor) {
    case AnchorTop:
        item->setX(anchorPoint.x() - sceneBoundingRect.width() / 2);
        item->setY(anchorPoint.y());
        break;
    case AnchorBottom:
        item->setX(anchorPoint.x() - sceneBoundingRect.width() / 2);
        item->setY(anchorPoint.y() - sceneBoundingRect.height());
        break;
    case AnchorLeft:
        item->setX(anchorPoint.x());
        item->setY(anchorPoint.y() - sceneBoundingRect.height() / 2);
        break;
    case AnchorRight:
        item->setX(anchorPoint.x() - sceneBoundingRect.width());
        item->setY(anchorPoint.y() - sceneBoundingRect.height() / 2);
        break;
    case AnchorTopLeft:
        item->setX(anchorPoint.x());
        item->setY(anchorPoint.y());
        break;
    case AnchorTopRight:
        item->setX(anchorPoint.x() - sceneBoundingRect.width());
        item->setY(anchorPoint.y());
        break;
    case AnchorBottomLeft:
        item->setX(anchorPoint.x());
        item->setY(anchorPoint.y() - sceneBoundingRect.height());
        break;
    case AnchorBottomRight:
        item->setX(anchorPoint.x() - sceneBoundingRect.width());
        item->setY(anchorPoint.y() - sceneBoundingRect.height());
        break;
    case AnchorCenter:
        item->setX(anchorPoint.x() - sceneBoundingRect.width() / 2);
        item->setY(anchorPoint.y() - sceneBoundingRect.height() / 2);
        break;
    }
}


static QPointF
bottomCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.bottom());
}
