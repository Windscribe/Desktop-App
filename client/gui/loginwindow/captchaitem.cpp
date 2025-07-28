#include "captchaitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "utils/log/categories.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

const int CAPTCHA_WIDTH = 274;
const int kSliderOffsetY = 12;
const int kSliderHeight = 24;
const QColor kBottomSliderBackgroundColor(12, 15, 21);
const QColor kBottomSliderRunnerColor(117, 252, 137);
const int kBottomSliderRunnerRadius = 16;

CaptchaItem::CaptchaItem(QGraphicsObject *parent, const QString &background, const QString &slider, int top) : ScalableGraphicsObject(parent)
{
    captchaTrailX_.rset_capacity(kMaxTrailSize);
    captchaTrailY_.rset_capacity(kMaxTrailSize);
    backgroundBase64_ = background;
    sliderBase64_ = slider;
    recreatePixmaps();
    sliderTop_ = top * pixmapScale_ / sliderPixmap_.devicePixelRatio();
}

QRectF CaptchaItem::boundingRect() const
{
    return QRectF(0, 0, backgroundPixmap_.width() / backgroundPixmap_.devicePixelRatio(),
                  backgroundPixmap_.height() / backgroundPixmap_.devicePixelRatio() + (kSliderHeight + kSliderOffsetY * 2) * G_SCALE);
}

void CaptchaItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();
    painter->setRenderHint(QPainter::Antialiasing);
    auto rc = boundingRect();
    painter->setClipRect(rc);
    QColor color = FontManager::instance().getDarkBlueColor();
    painter->fillRect(rc,QBrush(color));

    if (backgroundPixmap_.isNull() || sliderPixmap_.isNull())
        return;

    QPen pen(QColor(35, 45, 57));
    painter->setPen(pen);
    painter->setBrush(kBottomSliderBackgroundColor);
    QRectF rcBackgroundImage(0, 0, backgroundPixmap_.width() / backgroundPixmap_.devicePixelRatio(), backgroundPixmap_.height() / backgroundPixmap_.devicePixelRatio());
    painter->drawRoundedRect(rcBackgroundImage, 8*G_SCALE, 8*G_SCALE);

    // create the rounded rectangle clip area
    QPainterPath path;
    path.addRoundedRect(rcBackgroundImage.adjusted(1, 1, -1, -1), 8*G_SCALE, 8*G_SCALE);
    painter->setClipPath(path);

    // draw captcha
    painter->drawPixmap(0, 0, backgroundPixmap_);
    painter->drawPixmap(sliderLeft_, sliderTop_, sliderPixmap_);

    painter->setClipping(false);   // clear clip area

    // draw bottom slider background
    painter->setPen(pen);
    painter->setBrush(kBottomSliderBackgroundColor);
    auto rcSliderBackground = sliderBackgroundRect();
    painter->drawRoundedRect(rcSliderBackground, 10*G_SCALE, 10*G_SCALE);

    // draw text over the slider background
    painter->save();
    painter->setOpacity(initOpacity*0.5);
    QFont font = FontManager::instance().getFont(12, QFont::Normal);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(rcSliderBackground, Qt::AlignVCenter | Qt::AlignHCenter, tr("Slide puzzle piece into place"));
    painter->restore();


    // draw bottom slider runner
    painter->setPen(kBottomSliderRunnerColor);
    painter->setBrush(kBottomSliderRunnerColor);
    auto rcSliderRunner = sliderRunnerRect();
    painter->drawEllipse(rcSliderRunner.center(), kBottomSliderRunnerRadius*G_SCALE, kBottomSliderRunnerRadius*G_SCALE);

    // draw runner arrow
    auto img = ImageResourcesSvg::instance().getIndependentPixmap("login/SLIDER_ARROW_ICON");
    img->draw(rcSliderRunner.left() + (rcSliderRunner.width() - img->width()) / 2.0, rcSliderRunner.top() + (rcSliderRunner.height() - img->height()) / 2.0, painter);

    painter->setClipping(false);
}

void CaptchaItem::setClickable(bool enabled)
{
    setAcceptHoverEvents(enabled);
    QCursor cursor = Qt::ArrowCursor;
    if (enabled) cursor = Qt::PointingHandCursor;
    setCursor(cursor);
}

void CaptchaItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recreatePixmaps();
    update();
}

void CaptchaItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (inCaptchaArea(event->pos()) || inSliderArea(event->pos())) {
        mousePressedPos_ = event->pos();
    } else {
        ScalableGraphicsObject::mousePressEvent(event);
    }
}

void CaptchaItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mousePressedPos_.has_value()) {
        mousePressedPos_.reset();
        // convert to captcha background coordinates
        double captchaSolution = sliderLeft_ * backgroundPixmap_.devicePixelRatio() / pixmapScale_;
        emit captchaResolved(QString::number(static_cast<int>(captchaSolution)),
                             std::vector<float>(captchaTrailX_.begin(), captchaTrailX_.end()),
                             std::vector<float>(captchaTrailY_.begin(), captchaTrailY_.end()));
    }
    ScalableGraphicsObject::mouseReleaseEvent(event);
}

void CaptchaItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (mousePressedPos_.has_value()) {
        sliderLeft_ = event->pos().x() - mousePressedPos_->x();
        sliderLeft_ = std::max(0, sliderLeft_);
        sliderLeft_ = std::min((double)(CAPTCHA_WIDTH - 2*kBottomSliderRunnerRadius) * G_SCALE, (double)sliderLeft_);
        // push if at least one coordinate differs by more than 0.5 from the previous one
        if (captchaTrailX_.empty() || (abs(captchaTrailX_.back() - event->pos().x()) > 0.49) ||
            (abs(captchaTrailY_.back() - event->pos().y()) > 0.49)) {
            captchaTrailX_.push_back(event->pos().x());
            captchaTrailY_.push_back(event->pos().y());
        }
        update();
    } else {
        ScalableGraphicsObject::mouseMoveEvent(event);
    }
}

void CaptchaItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (inCaptchaArea(event->pos()) || inSliderArea(event->pos())) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

bool CaptchaItem::inCaptchaArea(const QPointF &pt) const
{
    QRectF rc(sliderLeft_, sliderTop_,
              sliderPixmap_.width() / sliderPixmap_.devicePixelRatio(), sliderPixmap_.height() / sliderPixmap_.devicePixelRatio());
    return rc.contains(pt);
}

bool CaptchaItem::inSliderArea(const QPointF &pt) const
{
    return sliderRunnerRect().contains(pt);
}

void CaptchaItem::recreatePixmaps()
{
    // Save top value in captcha coordinates
    std::optional<int> originalTop;
    if (!backgroundPixmap_.isNull()) {
        originalTop = sliderTop_ * backgroundPixmap_.devicePixelRatio() / pixmapScale_;
    }

    // Create images from Base64 strings and do scaling
    if (!backgroundPixmap_.loadFromData(QByteArray::fromBase64(backgroundBase64_.toUtf8()), "PNG")) {
        qCWarning(LOG_BASIC) << "CaptchaWindowItem::setCaptcha failed load PNG for background";
    }
    backgroundPixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    int initialWidth = backgroundPixmap_.width();
    backgroundPixmap_ = backgroundPixmap_.scaledToWidth(CAPTCHA_WIDTH * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio(), Qt::SmoothTransformation);

    pixmapScale_ = (double)backgroundPixmap_.width() / (double)initialWidth;

    if (!sliderPixmap_.loadFromData(QByteArray::fromBase64(sliderBase64_.toUtf8()), "PNG")) {
        qCWarning(LOG_BASIC) << "CaptchaWindowItem::setCaptcha failed load PNG for slider";
    }
    sliderPixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    sliderPixmap_ = sliderPixmap_.scaledToWidth(sliderPixmap_.width() * pixmapScale_, Qt::SmoothTransformation);

    // recalculate silder_Top in new coordinates
    if (originalTop.has_value()) {
        sliderTop_ = originalTop.value() * pixmapScale_ / sliderPixmap_.devicePixelRatio();
    }
}

QRectF CaptchaItem::sliderBackgroundRect() const
{
    auto rc = boundingRect();
    QRectF rcSliderBackground(0, rc.height() - (kSliderHeight + kSliderOffsetY) * G_SCALE, rc.width(), kSliderHeight*G_SCALE);
    return rcSliderBackground;
}

QRectF CaptchaItem::sliderRunnerRect() const
{
    auto rc = sliderBackgroundRect();
    QRectF rcSliderRunner(sliderLeft_, rc.center().y() - kBottomSliderRunnerRadius*G_SCALE, 2*kBottomSliderRunnerRadius*G_SCALE, 2*kBottomSliderRunnerRadius*G_SCALE);
    return rcSliderRunner;
}

}
