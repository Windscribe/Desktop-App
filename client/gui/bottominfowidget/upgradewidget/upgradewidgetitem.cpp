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
  , curDataIconPath_("DATA_OVER_ICON")
  , curLeftTextOffset_(LEFT_TEXT_WITH_ICON_OFFSET)
  , roundedRectXRadius_(10)
{
    QString buttonText = tr("GET MORE DATA");
    textButton_ = new CommonGraphics::TextButton(buttonText, fontDescr_, Qt::white, true, this, 15);
    textButton_->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(textButton_, &CommonGraphics::TextButton::clicked, this, &UpgradeWidgetItem::onButtonClick);

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

    // version text
    painter->save();
    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);
    painter->translate(0, fm.height()+1*G_SCALE);
    painter->setPen(curDataRemainingColor_);
    painter->setFont(font);
    painter->setOpacity(curDataRemainingTextOpacity_ * initOpacity);
    painter->drawText(curLeftTextOffset_*G_SCALE, 0, currentText());
    painter->restore();

    if (!isExtConfigMode_)
    {
        // Data Icon
        painter->setOpacity(curDataRemainingIconOpacity_ * initOpacity);
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(curDataIconPath_);
        pixmap->draw(3*G_SCALE+0.5, 3*G_SCALE+0.5, painter);
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

    if (modePro_)
    {
        if (daysLeft_ > 1)
        {
            dataColor = FontManager::instance().getBrightYellowColor();
        }
        else // 0, 1
        {
            dataColor = FontManager::instance().getErrorRedColor();
        }
    }
    else
    {
        qint64 bytesLeft = bytesMax_ - bytesUsed_;

        if (bytesLeft > TEN_GB_IN_BYTES/2) // 5+ GB
        {
            dataColor = FontManager::instance().getSeaGreenColor();
        }
        else if (bytesLeft > TEN_GB_IN_BYTES/10) // 1-4 GB
        {
            dataColor = FontManager::instance().getBrightYellowColor();
        }
        else // < 1GB
        {
            dataColor = FontManager::instance().getErrorRedColor();
        }
    }

    return dataColor;
}

void UpgradeWidgetItem::updateIconPath()
{
    qint64 bytesLeft = bytesMax_ - bytesUsed_;

    if (modePro_)
    {
        curDataIconPath_ = "RENEW_PRO_ICON";
    }
    else
    {
        if (bytesLeft > TEN_GB_IN_BYTES/2) // 5GB+
        {
            curDataIconPath_ = "DATA_FULL_ICON";
        }
        else if (bytesLeft > TEN_GB_IN_BYTES/10) // 1-4 GB
        {
            curDataIconPath_ = "DATA_LOW_ICON";
        }
        else // < 1GB
        {
            curDataIconPath_ = "DATA_OVER_ICON";
        }
    }
}

void UpgradeWidgetItem::setClickable(bool enable)
{
    if (textButton_ != nullptr)
    {
        textButton_->setClickable(enable);
    }
}

void UpgradeWidgetItem::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    if (!isExtConfigMode_)
    {
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
    if (!isExtConfigMode_)
    {
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
    // Constrain the text button to the remaining space in this widget.
    QFont font = FontManager::instance().getFont(fontDescr_);
    int currentTextWidth = CommonGraphics::textWidth(currentText(), font);
    int nAvailableWidth = width_*G_SCALE - curLeftTextOffset_*G_SCALE - currentTextWidth - 20*G_SCALE;
    textButton_->setMaxWidth(nAvailableWidth);

    textButton_->recalcBoundingRect();

    int updatePosX = curLeftTextOffset_*G_SCALE + currentTextWidth + 10*G_SCALE;
    textButton_->setPos(updatePosX, (int)(G_SCALE+0.5));
}

QString UpgradeWidgetItem::currentText()
{
    QString result = "";

    if (isExtConfigMode_)
    {
        result = tr("EXT CONFIG MODE");
    }
    else
    {
        if (modePro_)
        {
            result = makeDaysLeftString(daysLeft_);
        }
        else
        {
            const auto bytes_remaining = qMax(qint64(0), bytesMax_ - bytesUsed_);
            QLocale locale(LanguageController::instance().getLanguage());
            result = locale.formattedDataSize(bytes_remaining, 1, QLocale::DataSizeTraditionalFormat);
        }
    }

    return result;
}

QString UpgradeWidgetItem::makeDaysLeftString(int days)
{
    if (days == 0)
        return tr("0 DAYS LEFT");
    else if (days == 1)
        return tr("1 DAY LEFT");
    else if (days == 2)
        return tr("2 DAYS LEFT");
    else if (days == 3)
        return tr("3 DAYS LEFT");
    else if (days == 4)
        return tr("4 DAYS LEFT");
    else if (days == 5)
        return tr("5 DAYS LEFT");
    else
    {
        WS_ASSERT(false);
        return tr("%1 DAYS LEFT").arg(QString::number(days));
    }
}

void UpgradeWidgetItem::updateButtonText()
{
    if (isExtConfigMode_)
    {
        QString buttonText = tr("LOGIN");
        textButton_->setText(buttonText);
    }
    else
    {
        if (modePro_)
        {
            textButton_->setText(tr("RENEW"));
        }
        else
        {
            textButton_->setText(tr("GET MORE DATA"));
        }
    }
}

void UpgradeWidgetItem::setExtConfigMode(bool isExtConfigMode)
{
    if (isExtConfigMode != isExtConfigMode_)
    {
        isExtConfigMode_ = isExtConfigMode;

        if (isExtConfigMode)
        {
            curLeftTextOffset_ = LEFT_TEXT_NO_ICON_OFFSET;
            curDataRemainingColor_ = Qt::white;
        }
        else
        {
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
