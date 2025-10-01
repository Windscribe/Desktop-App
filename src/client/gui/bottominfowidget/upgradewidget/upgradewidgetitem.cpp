#include "upgradewidgetitem.h"

#include <QLocale>
#include <QPainter>
#include <QTimer>

#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"

namespace UpgradeWidget {

UpgradeWidgetItem::UpgradeWidgetItem(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
  , daysLeft_(-1)
  , bytesUsed_(-1)
  , bytesMax_(-1)
  , modePro_(false)
  , isExtConfigMode_(false)
  , curDataIconPath_("")
  , curLeftTextOffset_(LEFT_TEXT_WITH_ICON_OFFSET)
  , roundedRectXRadius_(10)
{
    QString buttonText = tr("Get more data");
    textButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kBanner, 15, HEIGHT_-2, HEIGHT_/2);
    textButton_->setFont(FontDescr(12, QFont::Medium));

    connect(textButton_, &CommonGraphics::BubbleButton::clicked, this, &UpgradeWidgetItem::onButtonClick);

    curBackgroundOpacity_ = OPACITY_FULL;
    curDataRemainingIconOpacity_ = OPACITY_FULL;
    curDataRemainingTextOpacity_ = OPACITY_FULL;

    curDataRemainingColor_ = FontManager::instance().getErrorRedColor();

    connect(&backgroundOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpgradeWidgetItem::onBackgroundOpacityChanged);
    connect(&dataRemainingTextOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpgradeWidgetItem::onDataRemainingTextOpacityChanged);
    connect(&dataRemainingIconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpgradeWidgetItem::onDataRemainingIconOpacityChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &UpgradeWidgetItem::onLanguageChanged);
    updateDisplayElements();
    updateTextPos();
}

QRectF UpgradeWidgetItem::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, HEIGHT_*G_SCALE);
}

void UpgradeWidgetItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(255, 0, 0)));

    painter->setRenderHint(QPainter::Antialiasing);

    qreal initOpacity = painter->opacity();

    // background
    painter->save();
    painter->setBrush(FontManager::instance().getMidnightColor());
    painter->setPen(FontManager::instance().getMidnightColor());
    painter->setOpacity(curBackgroundOpacity_ * initOpacity);
    QRectF rect(1*G_SCALE, 1*G_SCALE, (width_ - 2)*G_SCALE, (HEIGHT_ - 2)*G_SCALE);
    painter->drawRoundedRect(rect, roundedRectXRadius_, 100, Qt::RelativeSize);
    painter->restore();

    // data text
    painter->save();
    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);
    painter->translate(0, fm.height()+1*G_SCALE);
    painter->setPen(curDataRemainingColor_);
    painter->setFont(font);
    painter->setOpacity(curDataRemainingTextOpacity_ * initOpacity);
    painter->drawText(curLeftTextOffset_*G_SCALE, 0, currentText());
    painter->restore();

    if (!isExtConfigMode_) {
        if (!curDataIconPath_.isEmpty()) {
            // Data Icon
            painter->setOpacity(curDataRemainingIconOpacity_ * initOpacity);
            QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(curDataIconPath_);
            pixmap->draw(3*G_SCALE+0.5, 3*G_SCALE+0.5, painter);
        } else {
            // Draw background arc
            QPen pen = painter->pen();
            pen.setWidth(2*G_SCALE);
            pen.setColor(QColor(255, 255, 255, 51));

            painter->setPen(pen);
            painter->drawArc(QRectF(3*G_SCALE, 3*G_SCALE, 16*G_SCALE, 16*G_SCALE), 180*16, -360*16);

            // Draw unused arc
            painter->setPen(curDataRemainingColor_);
            double pct = 1 - (double)bytesUsed_/(double)bytesMax_;
            painter->drawArc(QRectF(3*G_SCALE, 3*G_SCALE, 16*G_SCALE, 16*G_SCALE), 180*16, -pct*360*16);

        }
    }
}

QPixmap UpgradeWidgetItem::getCurrentPixmapShape()
{
    QPixmap pixmap(width_*G_SCALE, HEIGHT_*G_SCALE);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setClipRect(0, HEIGHT_ / 2 * G_SCALE, width_*G_SCALE, (HEIGHT_ - HEIGHT_ / 2)*G_SCALE);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(FontManager::instance().getMidnightColor());
    painter.setPen(FontManager::instance().getMidnightColor());

    QRectF rect(1*G_SCALE, 1*G_SCALE, (width_ - 2)*G_SCALE, (HEIGHT_ - 2)*G_SCALE);
    painter.drawRoundedRect(rect, roundedRectXRadius_*G_SCALE, 100*G_SCALE, Qt::RelativeSize);
    painter.end();

    return pixmap;
}

void UpgradeWidgetItem::onLanguageChanged()
{
    updateTextPos();
}

QColor UpgradeWidgetItem::getTextColor()
{
    QColor dataColor;

    if (modePro_) {
        if (daysLeft_ > 1) {
            dataColor = FontManager::instance().getBrightYellowColor();
        } else { // 0, 1
            dataColor = FontManager::instance().getErrorRedColor();
        }
    } else {
        qint64 bytesLeft = bytesMax_ - bytesUsed_;

        if (bytesLeft > TEN_GB_IN_BYTES/2) { // 5+ GB
            dataColor = FontManager::instance().getSeaGreenColor();
        } else if (bytesLeft > TEN_GB_IN_BYTES/10) { // 1-4 GB
            dataColor = FontManager::instance().getBrightYellowColor();
        } else { // < 1GB
            dataColor = FontManager::instance().getErrorRedColor();
        }
    }

    return dataColor;
}

void UpgradeWidgetItem::updateIconPath()
{
    if (modePro_) {
        curDataIconPath_ = "RENEW_PRO_ICON";
    } else {
        curDataIconPath_ = "";
    }
}

void UpgradeWidgetItem::setClickable(bool enable)
{
    if (textButton_ != nullptr) {
        textButton_->setClickable(enable);
    }
}

void UpgradeWidgetItem::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    if (!isExtConfigMode_) {
        modePro_ = false;

        bytesUsed_ = bytesUsed;
        bytesMax_ = bytesMax;

        updateDisplayElements();
    }
}

void UpgradeWidgetItem::updateDisplayElements()
{
    updateButtonText();
    updateIconPath();
    curDataRemainingColor_ = getTextColor();
    update();
}

void UpgradeWidgetItem::setDaysRemaining(int days)
{
    if (!isExtConfigMode_) {
        modePro_ = true;
        daysLeft_ = days;

        updateDisplayElements();
    }
}

void UpgradeWidgetItem::onBackgroundOpacityChanged(const QVariant &value)
{
    curBackgroundOpacity_ = value.toDouble();
    update();
}

void UpgradeWidgetItem::onDataRemainingTextOpacityChanged(const QVariant &value)
{
    curDataRemainingTextOpacity_ = value.toDouble();
    update();
}

void UpgradeWidgetItem::onDataRemainingIconOpacityChanged(const QVariant &value)
{
    curDataRemainingIconOpacity_ = value.toDouble();
    update();
}

void UpgradeWidgetItem::onButtonClick()
{
    emit buttonClick();
}

void UpgradeWidgetItem::animateShow()
{
    startAnAnimation<double>(backgroundOpacityAnimation_, curBackgroundOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(dataRemainingTextOpacityAnimation_, curDataRemainingTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);

    startAnAnimation<double>(dataRemainingIconOpacityAnimation_, curDataRemainingIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);

    textButton_->animateShow(ANIMATION_SPEED_FAST);
}

void UpgradeWidgetItem::animateHide()
{
    startAnAnimation<double>(backgroundOpacityAnimation_, curBackgroundOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(dataRemainingTextOpacityAnimation_, curDataRemainingTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);

    startAnAnimation<double>(dataRemainingIconOpacityAnimation_, curDataRemainingIconOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);

    textButton_->animateHide(ANIMATION_SPEED_FAST);
}

void UpgradeWidgetItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updateTextPos();
}

void UpgradeWidgetItem::updateTextPos()
{
    QFont font = FontManager::instance().getFont(fontDescr_);
    int width = CommonGraphics::textWidth(tr("Get more data"), font) + 16*G_SCALE;
    textButton_->setWidth(width/G_SCALE);
    textButton_->setPos(width_*G_SCALE - textButton_->boundingRect().width(), 1*G_SCALE);
}

QString UpgradeWidgetItem::currentText()
{
    QString result = "";

    if (!isExtConfigMode_) {
        if (modePro_) {
            result = makeDaysLeftString(daysLeft_);
        } else {
            const auto bytes_remaining = qMax(qint64(0), bytesMax_ - bytesUsed_);
            QLocale locale(LanguageController::instance().getLanguage());
            result = tr("%1 left").arg(locale.formattedDataSize(bytes_remaining, 1, QLocale::DataSizeTraditionalFormat));
        }
    }

    return result;
}

QString UpgradeWidgetItem::makeDaysLeftString(int days)
{
    if (days == 0)
        return tr("0 days left");
    else if (days == 1)
        return tr("1 day left");
    else if (days == 2)
        return tr("2 days left");
    else if (days == 3)
        return tr("3 days left");
    else if (days == 4)
        return tr("4 days left");
    else if (days == 5)
        return tr("5 days left");
    else
    {
        WS_ASSERT(false);
        return tr("%1 days left").arg(QString::number(days));
    }
}

void UpgradeWidgetItem::updateButtonText()
{
    if (isExtConfigMode_) {
        QString buttonText = tr("Login");
        textButton_->setText(buttonText);
    } else {
        if (modePro_) {
            textButton_->setText(tr("Renew"));
        } else {
            textButton_->setText(tr("Get more data"));
        }
    }
}

void UpgradeWidgetItem::setExtConfigMode(bool isExtConfigMode)
{
    if (isExtConfigMode != isExtConfigMode_)
    {
        isExtConfigMode_ = isExtConfigMode;

        if (isExtConfigMode) {
            curLeftTextOffset_ = LEFT_TEXT_NO_ICON_OFFSET;
            curDataRemainingColor_ = Qt::white;
        } else {
            curLeftTextOffset_ = LEFT_TEXT_WITH_ICON_OFFSET;
            curDataRemainingColor_ = getTextColor();
            updateIconPath();
        }

        updateButtonText();

        updateTextPos();
        update();
    }
}

void UpgradeWidgetItem::setWidth(int newWidth)
{
    prepareGeometryChange();
    width_ = newWidth;

    roundedRectXRadius_ = 10;
    if (width_ > 300 ) roundedRectXRadius_ = 7;

    updateTextPos();
}

}
