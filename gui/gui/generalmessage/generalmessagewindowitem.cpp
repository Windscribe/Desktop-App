#include "generalmessagewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "CommonGraphics/commongraphics.h"
#include "GraphicResources/fontmanager.h"
#include "GraphicResources/imageresourcessvg.h"

namespace GeneralMessage {

GeneralMessageWindowItem::GeneralMessageWindowItem(bool errorMode, QGraphicsObject *parent) : ScalableGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    curBackgroundOpacity_       = OPACITY_FULL;
    curTitleOpacity_            = OPACITY_FULL;
    curDescriptionOpacity_      = OPACITY_FULL;

#ifdef Q_OS_WIN
    background_ = "background/WIN_MAIN_BG";
#else
    background_ = "background/MAC_MAIN_BG";
#endif

    QString acceptText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonDark", "Ok");

    acceptButton_ = new CommonGraphics::BubbleButtonDark(this, 108, 40, 20, 20);
    connect(acceptButton_, SIGNAL(clicked()), this, SLOT(onAcceptClick()));
    acceptButton_->setText(acceptText);
    const int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, ACCEPT_BUTTON_POS_Y);

    curTitleText_ = "General Message Title";
    curDescriptionText_ = "General Message Description";

    if (errorMode) // ERROR
    {
        curTitleText_ = "General Error Title";
        curDescriptionText_ = "General Error Description";
    }

    setErrorMode(errorMode);
}

QRectF GeneralMessageWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void GeneralMessageWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    qreal initialOpacity = painter->opacity();

    // background:
    painter->setOpacity(curBackgroundOpacity_ * initialOpacity);
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(background_);
    p->draw(0, 0, painter);

    // title
    painter->setOpacity(curTitleOpacity_ * initialOpacity);
    painter->setPen(titleColor_);

    QFont titleFont = *FontManager::instance().getFont(24, true);
    painter->setFont(titleFont);

    QRectF titleRect(0, TITLE_POS_Y, WINDOW_WIDTH, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr(curTitleText_.toStdString().c_str()));

    // main description
    painter->setOpacity(curDescriptionOpacity_ * initialOpacity);
    QFont descFont(*FontManager::instance().getFont(14, false));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    QRect descRect = CommonGraphics::idealRect(0,0, DESCRIPTION_WIDTH_MIN, WINDOW_WIDTH - 32, 3, tr(curDescriptionText_.toStdString().c_str()), descFont, Qt::TextWordWrap);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH, descRect.width()), DESCRIPTION_POS_Y - descRect.height()/2,
                      descRect.width(), WINDOW_HEIGHT,
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


}
