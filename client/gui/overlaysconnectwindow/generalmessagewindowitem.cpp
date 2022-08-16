#include "generalmessagewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GeneralMessage {

GeneralMessageWindowItem::GeneralMessageWindowItem(Preferences *preferences, bool errorMode, QGraphicsObject *parent)
  : ScalableGraphicsObject(parent), preferences_(preferences), height_(WINDOW_HEIGHT)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    curBackgroundOpacity_       = OPACITY_FULL;
    curTitleOpacity_            = OPACITY_FULL;
    curDescriptionOpacity_      = OPACITY_FULL;

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    background_ = "background/WIN_MAIN_BG";
#else
    background_ = "background/MAC_MAIN_BG";
#endif

    QString acceptText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonDark", "Ok");

    connect(preferences_, &Preferences::appSkinChanged, this, &GeneralMessageWindowItem::onAppSkinChanged);

    acceptButton_ = new CommonGraphics::BubbleButtonDark(this, 108, 40, 20, 20);
    connect(acceptButton_, &CommonGraphics::BubbleButtonDark::clicked, this, &GeneralMessageWindowItem::onAcceptClick);
    acceptButton_->setText(acceptText);

    curTitleText_ = "General Message Title";
    curDescriptionText_ = "General Message Description";

    if (errorMode) // ERROR
    {
        curTitleText_ = "General Error Title";
        curDescriptionText_ = "General Error Description";
    }

    setErrorMode(errorMode);
    updateScaling();
}

QRectF GeneralMessageWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, height_*G_SCALE);
}

void GeneralMessageWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    qreal initialOpacity = painter->opacity();

    // background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        painter->setPen(Qt::NoPen);
#ifdef Q_OS_MAC
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 5*G_SCALE, 5*G_SCALE);
        painter->fillPath(path, QColor(2, 13, 28));
#else
        painter->fillRect(boundingRect().toRect(), QColor(2, 13, 28));
#endif
        painter->setPen(Qt::SolidLine);
    }
    else
    {
        painter->setOpacity(curBackgroundOpacity_ * initialOpacity);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(background_);
        p->draw(0, 0, painter);
    }

    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -28*G_SCALE : 0;

    // title
    painter->setOpacity(curTitleOpacity_ * initialOpacity);
    painter->setPen(titleColor_);

    QFont titleFont = *FontManager::instance().getFont(24, true);
    painter->setFont(titleFont);

    QRectF titleRect(0, (TITLE_POS_Y + yOffset)*G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr(curTitleText_.toStdString().c_str()));

    // main description
    painter->setOpacity(curDescriptionOpacity_ * initialOpacity);
    QFont descFont(*FontManager::instance().getFont(14, false));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    QRect descRect = CommonGraphics::idealRect(0,0, DESCRIPTION_WIDTH_MIN*G_SCALE, (WINDOW_WIDTH - 32)*G_SCALE, 3, tr(curDescriptionText_.toStdString().c_str()), descFont, Qt::TextWordWrap);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, descRect.width()), (DESCRIPTION_POS_Y + yOffset)*G_SCALE - descRect.height()/2,
                      descRect.width(), height_*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, tr(curDescriptionText_.toStdString().c_str()));
}

void GeneralMessageWindowItem::setTitle(QString title)
{
    curTitleText_ = title;
}

void GeneralMessageWindowItem::setDescription(QString description)
{
    curDescriptionText_ = description;
}

void GeneralMessageWindowItem::setErrorMode(bool error)
{
    if (error)
    {
        titleColor_ = FontManager::instance().getErrorRedColor();
    }
    else
    {
        titleColor_ = Qt::white;
    }
    update();
}

void GeneralMessageWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
}

void GeneralMessageWindowItem::updatePositions()
{
    const int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, acceptButton_->boundingRect().width());
    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -28*G_SCALE : 0;
    acceptButton_->setPos(acceptPosX, (ACCEPT_BUTTON_POS_Y + yOffset)*G_SCALE);
}

void GeneralMessageWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit acceptClick();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit acceptClick();
    }
}

void GeneralMessageWindowItem::onAcceptClick()
{
    emit acceptClick();
}

void GeneralMessageWindowItem::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    update();
}

void GeneralMessageWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    height_ = height;
    updatePositions();
}

}