#include "externalconfigwindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace ExternalConfigWindow {

ExternalConfigWindowItem::ExternalConfigWindowItem(QGraphicsObject *parent,
                                                   PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    Q_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curIconPath_ = "BIG_CONFIG_ICON";

    curBackgroundOpacity_ = OPACITY_FULL;
    curTextOpacity_ = OPACITY_FULL;
    curEscTextOpacity_ = OPACITY_FULL;

    QString acceptText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonDark", "Ok, got it!");

    okButton_ = new CommonGraphics::BubbleButtonDark(this, 108, 40, 20, 20);
    okButton_->setText(acceptText);
    connect(okButton_, SIGNAL(clicked()), this, SLOT(onButtonClicked()));

    escButton_ = new CommonGraphics::EscapeButton(this);
    connect(escButton_, SIGNAL(clicked()), SLOT(onEscClicked()));

#ifdef Q_OS_WIN
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
#else //if Q_OS_MAC
    /*closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setSelected(true);*/

#endif

    connect(preferencesHelper, SIGNAL(isDockedModeChanged(bool)), this,
            SLOT(onDockedModeChanged(bool)));

    updatePositions();
}

QGraphicsObject *ExternalConfigWindowItem::getGraphicsObject()
{
    return this;
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
#ifdef Q_OS_WIN
    painter->fillRect(boundingRect(), black);
#else
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(boundingRect().adjusted(0,0,0,0), 5*G_SCALE, 5*G_SCALE);
#endif

    painter->setPen(QColor(255,255,255));

    // Icon
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    //QPixmap *pixmap = ImageResourcesSvg::instance().getPixmap(curIconPath_);
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(curIconPath_);
    pixmap->draw((WINDOW_WIDTH/2 - 20)*G_SCALE, ICON_POS_Y*G_SCALE, painter);

    // title
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(16, true, 100));
    QRectF titleRect(0, TITLE_POS_Y*G_SCALE, LOGIN_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
    painter->drawText(titleRect, Qt::AlignHCenter, tr("External Config Mode"));

    // desc
    painter->setPen(QColor(255,255,255));

    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(14, false, 100));

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

#ifdef Q_OS_WIN
    closeButton_->setClickable(isClickable);
    minimizeButton_->setClickable(isClickable);
#endif
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

void ExternalConfigWindowItem::onDockedModeChanged(bool /*bIsDockedToTray*/)
{
#if defined(Q_OS_MAC)
    //minimizeButton_->setVisible(!bIsDockedToTray);
#endif
}

void ExternalConfigWindowItem::updatePositions()
{
#ifdef Q_OS_WIN
    closeButton_->setPos((LOGIN_WIDTH - 16 - WINDOW_MARGIN)*G_SCALE, 14*G_SCALE);
    minimizeButton_->setPos((LOGIN_WIDTH - 16 - WINDOW_MARGIN -32)*G_SCALE, 14*G_SCALE);
#else
    //minimizeButton_->setPos(28*G_SCALE, 8*G_SCALE);
    //closeButton_->setPos(    8*G_SCALE, 8*G_SCALE);
#endif

    escButton_->setPos(WINDOW_MARGIN*G_SCALE, WINDOW_MARGIN*G_SCALE);
    okButton_->setPos(WINDOW_WIDTH/2*G_SCALE - okButton_->boundingRect().width()/2, CONNECT_BUTTON_POS_Y*G_SCALE);
}

}
