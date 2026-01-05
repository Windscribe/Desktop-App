#include "locationsbutton.h"

#include <QBitmap>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "utils/utils.h"

namespace ConnectWindow {

LocationsButton::LocationsButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    arrowRotation_(0),
    background_("background/LOCATIONS_BG"),
    curTextColor_(Qt::white), isExpanded_(false),
    curOpacity_(OPACITY_SEVENTY)
{
    arrowItem_ = new ImageItem(this, "ARROW_DOWN_LOCATIONS");
    //arrowItem_->setTransformationMode(Qt::SmoothTransformation);
    arrowItem_->setOpacity(0.5);

    rotationAnimation_ = new QPropertyAnimation(this);
    rotationAnimation_->setTargetObject(this);
    rotationAnimation_->setPropertyName("arrowRotation");
    rotationAnimation_->setStartValue(0.0);
    rotationAnimation_->setEndValue(180.0);
    rotationAnimation_->setDuration(270);

    locationsMenu_ = new LocationsMenu(this);
    connect(locationsMenu_, &LocationsMenu::locationTabClicked, this, &LocationsButton::locationTabClicked);
    connect(locationsMenu_, &LocationsMenu::searchFilterChanged, this, &LocationsButton::searchFilterChanged);
    connect(locationsMenu_, &LocationsMenu::locationsKeyPressed, this, &LocationsButton::locationsKeyPressed);
    locationsMenu_->hide();

    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &LocationsButton::onOpacityValueChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LocationsButton::onLanguageChanged);

    setClickableHoverable(true, true);
    connect(this, &ClickableGraphicsObject::hoverEnter, this, &LocationsButton::onHoverEnter);
    connect(this, &ClickableGraphicsObject::hoverLeave, this, &LocationsButton::onHoverLeave);

    updatePositions();
}

QRectF LocationsButton::boundingRect() const
{
    return QRectF(0, 0, 252 * G_SCALE, 58 * G_SCALE);
}

QPainterPath LocationsButton::shape() const
{
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(background_);

    QPainterPath path;
    path.addRegion(QRegion(pixmap->mask()));
    return path;
}

void LocationsButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 25, 255)));
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(background_);
    pixmap->draw(0, 0, painter);

    // background for the arrow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 26));
    painter->drawEllipse(212*G_SCALE, 16*G_SCALE, 24*G_SCALE, 24*G_SCALE);
    painter->setPen(Qt::SolidLine);

    if (!isExpanded_) {
        painter->setOpacity(curOpacity_);
        QFont font = FontManager::instance().getFont(15,  QFont::Normal);
        painter->setFont(font);
        painter->setPen(curTextColor_);
        painter->drawText(QRect(boundingRect().left() + 40*G_SCALE, boundingRect().top(), boundingRect().width(), boundingRect().height()), Qt::AlignVCenter, tr("Locations"));
    }
}

void LocationsButton::setArrowRotation(qreal r)
{
    arrowRotation_ = r;
    QTransform tr;
    tr.translate(0, 12*G_SCALE);
    tr.rotate(arrowRotation_, Qt::XAxis);
    tr.translate(0, -12*G_SCALE);
    arrowItem_->setTransform(tr);
    emit arrowRotationChanged();
}

void LocationsButton::onLocationsExpandStateChanged(bool isExpanded)
{
    if (isExpanded) {
        rotationAnimation_->setDirection(QPropertyAnimation::Forward);
        if (rotationAnimation_->state() != QPropertyAnimation::Running) {
            rotationAnimation_->start();
        }
        locationsMenu_->show();
    } else {
        rotationAnimation_->setDirection(QPropertyAnimation::Backward);
        if (rotationAnimation_->state() != QPropertyAnimation::Running) {
            rotationAnimation_->start();
        }
        locationsMenu_->dismiss();
        locationsMenu_->hide();
    }
    isExpanded_ = isExpanded;
    curOpacity_ = isExpanded ? OPACITY_FULL : OPACITY_SEVENTY;
}

void LocationsButton::setTextColor(QColor color)
{
    curTextColor_ = color;
    update();
}

void LocationsButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    locationsMenu_->setPos(32*G_SCALE, 12*G_SCALE);
    updatePositions();
}


void LocationsButton::updatePositions()
{
    arrowItem_->setPos(212*G_SCALE, 16*G_SCALE);
    setArrowRotation(isExpanded_ ? 180 : 0);
}

void LocationsButton::onHoverEnter()
{
    if (!isExpanded_) {
        startAnAnimation<double>(opacityAnimation_, curOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
}

void LocationsButton::onHoverLeave()
{
    if (!isExpanded_) {
        startAnAnimation<double>(opacityAnimation_, curOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    }
}

void LocationsButton::onOpacityValueChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    arrowItem_->setOpacity(curOpacity_);
    update();
}

void LocationsButton::onLanguageChanged()
{
    update();
}

bool LocationsButton::handleKeyPressEvent(QKeyEvent *event)
{
    if (!locationsMenu_->handleKeyPressEvent(event)) {
        if (event->key() == Qt::Key_Escape && isExpanded_) {
            emit clicked();
            return true;
        }
        return false;
    }
    return true;
}

void LocationsButton::setIsExternalConfigMode(bool isExternalConfigMode)
{
    locationsMenu_->setIsExternalConfigMode(isExternalConfigMode);
}

} //namespace ConnectWindow
