#include "externalconfigwindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"

namespace ExternalConfigWindow {

ExternalConfigWindowItem::ExternalConfigWindowItem(QGraphicsObject *parent,
                                                   PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curIconPath_ = "BIG_CONFIG_ICON";

    curBackgroundOpacity_ = OPACITY_FULL;
    curTextOpacity_ = OPACITY_FULL;
    curEscTextOpacity_ = OPACITY_FULL;

    okButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kOutline, 108, 40, 20);
    connect(okButton_, &CommonGraphics::BubbleButton::clicked, this, &ExternalConfigWindowItem::onButtonClicked);

    escButton_ = new CommonGraphics::EscapeButton(this);
    connect(escButton_, &CommonGraphics::EscapeButton::clicked, this, &ExternalConfigWindowItem::onEscClicked);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ExternalConfigWindowItem::onLanguageChanged);
    onLanguageChanged();

    updatePositions();
}

QRectF ExternalConfigWindowItem::boundingRect() const
{
    return QRect(0,0,LOGIN_WIDTH*G_SCALE,LOGIN_HEIGHT*G_SCALE);
}

void ExternalConfigWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    painter->setOpacity(curBackgroundOpacity_ * initialOpacity);

    // background
    QColor black = FontManager::instance().getMidnightColor();
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(boundingRect(), 9*G_SCALE, 9*G_SCALE);

    painter->setPen(QColor(255,255,255));

    // Icon
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    //QPixmap *pixmap = ImageResourcesSvg::instance().getPixmap(curIconPath_);
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(curIconPath_);
    pixmap->draw((WINDOW_WIDTH/2 - 20)*G_SCALE, ICON_POS_Y*G_SCALE, painter);

    // title
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(FontManager::instance().getFont(16, QFont::Bold, 100));
    QRectF titleRect(0, TITLE_POS_Y*G_SCALE, LOGIN_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
    painter->drawText(titleRect, Qt::AlignHCenter, tr("External Config Mode"));

    // desc
    painter->setPen(QColor(255,255,255));

    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(FontManager::instance().getFont(14, QFont::Normal, 100));

    QString descriptionText = tr("Use the Windscribe app without an account to connect to any OpenVPN or WireGuard server.");
    QFontMetrics fm = painter->fontMetrics();
    int width = fm.horizontalAdvance(descriptionText)/3; // 3 lines
    if (width < DESCRIPTION_WIDTH_MIN*G_SCALE) width = DESCRIPTION_WIDTH_MIN*G_SCALE;
    else if (width > LOGIN_WIDTH*G_SCALE) width = LOGIN_WIDTH*G_SCALE;
    painter->drawText(CommonGraphics::centeredOffset(LOGIN_WIDTH*G_SCALE, width), DESCRIPTION_POS_Y*G_SCALE,
                      width, WINDOW_HEIGHT*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descriptionText);

}

void ExternalConfigWindowItem::setIcon(QString iconPath)
{
    curIconPath_ = iconPath;
}

void ExternalConfigWindowItem::setButtonText(QString text)
{
    okButton_->setText(text);
}

void ExternalConfigWindowItem::setClickable(bool isClickable)
{
    escButton_->setClickable(isClickable);
    okButton_->setClickable(isClickable);
}

void ExternalConfigWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void ExternalConfigWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escapeClick();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit buttonClick();
    }
}

void ExternalConfigWindowItem::onEscClicked()
{
    emit escapeClick();
}

void ExternalConfigWindowItem::onButtonClicked()
{
    emit buttonClick();
}

void ExternalConfigWindowItem::onBackgroundOpacityChange(const QVariant &value)
{
    curBackgroundOpacity_ = value.toDouble();
    update();
}

void ExternalConfigWindowItem::onTextOpacityChange(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void ExternalConfigWindowItem::onEscTextOpacityChange(const QVariant &value)
{
    curEscTextOpacity_ = value.toDouble();
    update();
}

void ExternalConfigWindowItem::updatePositions()
{
    escButton_->setPos(WINDOW_WIDTH*G_SCALE - escButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE, WINDOW_MARGIN*G_SCALE);
    okButton_->setPos(WINDOW_WIDTH/2*G_SCALE - okButton_->boundingRect().width()/2, CONNECT_BUTTON_POS_Y*G_SCALE);
}

void ExternalConfigWindowItem::onLanguageChanged()
{
    okButton_->setText(tr("Ok, got it!"));
}

}
