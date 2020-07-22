#include "locationsbutton.h"

#include <QBitmap>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "GraphicResources/imageresourcessvg.h"
#include "GraphicResources/fontmanager.h"
#include "CommonGraphics/commongraphics.h"
#include "Utils/utils.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {

LocationsButton::LocationsButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    arrowRotation_(0), isExpanded_(false)
{

    curTextColor_ = Qt::white;

#ifdef Q_OS_WIN
    backgroundDark_ = "background/WIN_LOCATION_BUTTON_BG_DARK";
    backgroundLight_ = "background/WIN_LOCATION_BUTTON_BG_BRIGHT";
#else
    backgroundDark_ = "background/MAC_LOCATION_BUTTON_BG_DARK";
    backgroundLight_ = "background/MAC_LOCATION_BUTTON_BG_BRIGHT";
#endif

    backgroundDarkOpacity_ = OPACITY_FULL;
    backgroundLightOpacity_ = OPACITY_HIDDEN;

    arrowItem_ = new ImageItem("ARROW_DOWN_WHITE", this);
    //arrowItem_->setTransformationMode(Qt::SmoothTransformation);
    arrowItem_->setOpacity(0.5);

    rotationAnimation_ = new QPropertyAnimation(this);
    rotationAnimation_->setTargetObject(this);
    rotationAnimation_->setPropertyName("arrowRotation");
    rotationAnimation_->setStartValue(0.0);
    rotationAnimation_->setEndValue(180.0);
    rotationAnimation_->setDuration(270);

    backgroundOpacityAnimation_.setStartValue(OPACITY_FULL);
    backgroundOpacityAnimation_.setEndValue(OPACITY_HIDDEN);
    backgroundOpacityAnimation_.setDuration(270);
    connect(&backgroundOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBackgroundOpacityChange(QVariant)));
    setClickable(true);
    updatePositions();
}

QRectF LocationsButton::boundingRect() const
{
    return QRectF(0, 0, 257 * G_SCALE, 48 * G_SCALE);
}

QPainterPath LocationsButton::shape() const
{
    IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(backgroundDark_);

    QPainterPath path;
    path.addRegion(QRegion(pixmap->mask()));
    return path;
}

void LocationsButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 25, 255)));
    double initOpacity = painter->opacity();
    painter->setOpacity(backgroundDarkOpacity_ * initOpacity);
    IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap(backgroundDark_);
    pixmap->draw(0, 0, painter);

    painter->setOpacity(backgroundLightOpacity_ * initOpacity);
    IndependentPixmap *pixmapLight = ImageResourcesSvg::instance().getIndependentPixmap(backgroundLight_);
    pixmapLight->draw(0, 0, painter);

    painter->setOpacity(initOpacity);
    QFont *font = FontManager::instance().getFont(16, false);
    painter->setFont(*font);
    painter->setPen(curTextColor_);
    painter->drawText(QRect(boundingRect().left()+40*G_SCALE, boundingRect().top(), boundingRect().width(), boundingRect().height() - 2*G_SCALE), Qt::AlignVCenter, tr("Locations"));
}

void LocationsButton::setArrowRotation(qreal r)
{
    arrowRotation_ = r;
    QTransform tr;
    tr.translate(0, 8 * G_SCALE);
    tr.rotate(arrowRotation_, Qt::XAxis);
    tr.translate(0, -8 * G_SCALE);
    arrowItem_->setTransform(tr);
    emit arrowRotationChanged();
}

void LocationsButton::onLocationsExpandStateChanged(bool isExpanded)
{
    if (isExpanded)
    {
        rotationAnimation_->setDirection(QPropertyAnimation::Forward);
        if (rotationAnimation_->state() != QPropertyAnimation::Running)
        {
            rotationAnimation_->start();
        }

        backgroundOpacityAnimation_.setDirection(QPropertyAnimation::Forward);
        if (backgroundOpacityAnimation_.state() != QVariantAnimation::Running)
        {
            backgroundOpacityAnimation_.start();
        }
    }
    else
    {
        rotationAnimation_->setDirection(QPropertyAnimation::Backward);
        if (rotationAnimation_->state() != QPropertyAnimation::Running)
        {
            rotationAnimation_->start();
        }

        backgroundOpacityAnimation_.setDirection(QPropertyAnimation::Backward);
        if (backgroundOpacityAnimation_.state() != QVariantAnimation::Running)
        {
            backgroundOpacityAnimation_.start();
        }
    }
    isExpanded_ = isExpanded;
}

void LocationsButton::setTextColor(QColor color)
{
    curTextColor_ = color;
    update();
}

void LocationsButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    updatePositions();
}


void LocationsButton::onBackgroundOpacityChange(const QVariant &value)
{
    backgroundDarkOpacity_ = value.toDouble();
    backgroundLightOpacity_ = OPACITY_FULL - value.toDouble();
    update();
}

void LocationsButton::updatePositions()
{
    arrowItem_->setPos(206*G_SCALE, 16*G_SCALE);
    setArrowRotation(isExpanded_ ? 180 : 0);
}

} //namespace ConnectWindow
