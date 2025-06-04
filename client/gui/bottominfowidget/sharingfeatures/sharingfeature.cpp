#include "sharingfeature.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace SharingFeatures {

SharingFeature::SharingFeature(QString ssidOrProxyText, QString primaryIcon, ScalableGraphicsObject *parent)
    : ScalableGraphicsObject(parent), primaryIcon_(primaryIcon),
    secondaryIcon_("sharingfeatures/USERS_ICON"), number_(0), rounded_(false), curDefaultOpacity_(0)
{
    textIconButton_ = new CommonGraphics::TextIconButton(SPACE_BETWEEN_TEXT_AND_ARROW, ssidOrProxyText, "sharingfeatures/FRWRD_ARROW_ICON", this, false);
    textIconButton_->setCursor(Qt::PointingHandCursor);
    textIconButton_->setVerticalOffset(-4);

    connect(textIconButton_, &CommonGraphics::TextIconButton::widthChanged, this, &SharingFeature::onTextIconWidthChanged);
    connect(textIconButton_, &CommonGraphics::TextIconButton::clicked, this, &SharingFeature::clicked);

    verticalLine_ = new CommonGraphics::VerticalDividerLine(this, 16);

    updatePositions();
}

QRectF SharingFeature::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, HEIGHT*G_SCALE);
}

void SharingFeature::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // background
    painter->setOpacity(curDefaultOpacity_);

    const int ROUNDED_OFFSET = 2*G_SCALE;
    QRectF background(0, -ROUNDED_OFFSET, WIDTH*G_SCALE, HEIGHT*G_SCALE + ROUNDED_OFFSET);

#ifdef Q_OS_MACOS
    //todo scaling
    if (rounded_)
    {
        painter->setPen(FontManager::instance().getMidnightColor());
        painter->setBrush(FontManager::instance().getMidnightColor());
        painter->drawRoundedRect(background, 5, 5);
    }
    else
    {
        painter->fillRect(background, FontManager::instance().getMidnightColor());
    }
#else
    painter->fillRect(background, FontManager::instance().getMidnightColor());
#endif
    const int MARGIN_X = 16;
    const int MARGIN_Y = 16;
    const int MAIN_ICON_POS_Y = 12;

    QSharedPointer<IndependentPixmap> primaryPixmap = ImageResourcesSvg::instance().getIndependentPixmap(primaryIcon_);
    primaryPixmap->draw(MARGIN_X*G_SCALE, MAIN_ICON_POS_Y*G_SCALE, painter);

    // Account Icon
    painter->setOpacity(OPACITY_UNHOVER_ICON_TEXT_DARK * curDefaultOpacity_);
    QSharedPointer<IndependentPixmap> secondaryPixmap = ImageResourcesSvg::instance().getIndependentPixmap(secondaryIcon_);
    secondaryPixmap->draw(SECONDARY_ICON_POS_X*G_SCALE, MARGIN_Y*G_SCALE, painter);

    // User number
    QFont font = FontManager::instance().getFont(16, QFont::Normal);
    QFontMetrics fontMetrics(font);
    painter->setOpacity(OPACITY_UNHOVER_TEXT * curDefaultOpacity_);
    painter->setPen(Qt::white);
    painter->setFont(font);
    painter->drawText(USER_NUMBER_POS_X*G_SCALE, (HEIGHT*G_SCALE - fontMetrics.height()) / 2 + fontMetrics.ascent() - 1*G_SCALE, QString::number(number_));
}

void SharingFeature::setText(QString text)
{
    textIconButton_->setText(text);
}

void SharingFeature::setPrimaryIcon(QString iconPath)
{
    primaryIcon_ = iconPath;
    update();
}

void SharingFeature::setNumber(int number)
{
    number_ = number;
    update();
}

void SharingFeature::setOpacityByFactor(double opacityFactor)
{
    curDefaultOpacity_ = opacityFactor;

    textIconButton_->setOpacityByFactor(opacityFactor);

    update();
}

void SharingFeature::setClickable(bool clickable)
{
    textIconButton_->setClickable(clickable);
}

void SharingFeature::setRounded(bool rounded)
{
    rounded_ = rounded;
    update();
}

void SharingFeature::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void SharingFeature::onTextIconWidthChanged(int newWidth)
{
    textIconButton_->setPos(WIDTH*G_SCALE - newWidth - 16*G_SCALE, textIconButton_->getHeight() - 4*G_SCALE);
}

void SharingFeature::updatePositions()
{
    textIconButton_->setPos(WIDTH*G_SCALE - textIconButton_->getWidth() - 16*G_SCALE, textIconButton_->getHeight() - 4*G_SCALE);
    verticalLine_->setPos(DIVIDER_POS_X*G_SCALE, 16*G_SCALE);

}

}
