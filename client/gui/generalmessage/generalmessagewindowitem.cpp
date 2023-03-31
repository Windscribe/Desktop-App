#include "generalmessagewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace GeneralMessageWindow {

GeneralMessageWindowItem::GeneralMessageWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper,
        IGeneralMessageWindow::Style style, const QString &icon, const QString &title, const QString &desc,
        const QString &acceptText, const QString &rejectText) :
    IGeneralMessageWindow(parent, preferences, preferencesHelper), isSpinnerMode_(false), shape_(IGeneralMessageWindow::kLoginScreenShape)
{
    setBackButtonEnabled(false);
    setResizeBarEnabled(false);

    contentItem_ = new GeneralMessageItem(this, WINDOW_WIDTH, style);
    contentItem_->setIcon(icon);
    contentItem_->setTitle(title);
    contentItem_->setDescription(desc);
    contentItem_->setAcceptText(acceptText);
    contentItem_->setRejectText(rejectText);
    scrollAreaItem_->setItem(contentItem_);

    // Do not go into Van Gogh mode if at login screen
    if (shape_ == IGeneralMessageWindow::kLoginScreenShape) {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_BOTTOM);
    }

    connect(contentItem_, &GeneralMessageItem::acceptClick, this, &GeneralMessageWindowItem::acceptClick);
    connect(contentItem_, &GeneralMessageItem::rejectClick, this, &GeneralMessageWindowItem::rejectClick);
    connect(&spinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSpinnerRotationChanged(QVariant)));
    connect(&spinnerRotationAnimation_, SIGNAL(finished()), SLOT(onSpinnerRotationFinished()));
    connect(this, &ResizableWindow::escape, this, &GeneralMessageWindowItem::onEscape);
}

void GeneralMessageWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // resize area background
    qreal initialOpacity = painter->opacity();
    painter->fillRect(boundingRect().adjusted(0, 286*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));

    // bottom-most background
    painter->setOpacity(initialOpacity);
    if (roundedFooter_) {
        painter->setPen(QColor(2, 13, 28));
        painter->setBrush(QColor(2, 13, 28));
        painter->drawRoundedRect(getBottomResizeArea(), 8*G_SCALE, 8*G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));
    } else {
        painter->fillRect(getBottomResizeArea(), QBrush(QColor(2, 13, 28)));
    }

    // base background
    if (shape_ == IGeneralMessageWindow::kConnectScreenAlphaShape) {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
    } else {
        QPainterPath path;
#ifdef Q_OS_MAC
        path.addRoundedRect(boundingRect().toRect(), 5*G_SCALE, 5*G_SCALE);
#else
        path.addRect(boundingRect().toRect());
#endif
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(2, 13, 28));
        painter->setPen(Qt::SolidLine);
    }

    if (isSpinnerMode_) {
        int offset = 0;
        if (shape_ == IGeneralMessageWindow::kConnectScreenAlphaShape) {
            offset = -14*G_SCALE;
        }

        // spinner
        painter->setPen(QPen(Qt::white, 2 * G_SCALE));
        painter->translate(boundingRect().width()/2, boundingRect().height()/2 + offset);
        painter->rotate(curSpinnerRotation_);
        const int circleDiameter = 80*G_SCALE;
        painter->drawArc(QRect(-circleDiameter/2, -circleDiameter/2, circleDiameter, circleDiameter), 0, 4 * 360);
    }
}

void GeneralMessageWindowItem::setTitle(const QString &title)
{
    contentItem_->setTitle(title);
    updateHeight();
}

void GeneralMessageWindowItem::setIcon(const QString &icon)
{
    contentItem_->setIcon(icon);
    updateHeight();
}

void GeneralMessageWindowItem::setDescription(const QString &desc)
{
    contentItem_->setDescription(desc);
    updateHeight();
}

void GeneralMessageWindowItem::setAcceptText(const QString &text)
{
    contentItem_->setAcceptText(text);
    updateHeight();
}

void GeneralMessageWindowItem::setRejectText(const QString &text)
{
    contentItem_->setRejectText(text);
    updateHeight();
}

void GeneralMessageWindowItem::setTitleSize(int size)
{
    contentItem_->setTitleSize(size);
    updateHeight();
}

void GeneralMessageWindowItem::setBackgroundShape(IGeneralMessageWindow::Shape shape)
{
    shape_ = shape;
    updatePositions();
    updateHeight();
}

void GeneralMessageWindowItem::onEscape()
{
    if (!isSpinnerMode_) {
        emit rejectClick();
    }
}

void GeneralMessageWindowItem::onAppSkinChanged(APP_SKIN s)
{
    if (shape_ != IGeneralMessageWindow::kLoginScreenShape) {
        ResizableWindow::onAppSkinChanged(s);
    }
}

void GeneralMessageWindowItem::setSpinnerMode(bool on)
{
    isSpinnerMode_ = on;

    scrollAreaItem_->setVisible(!isSpinnerMode_);
    escapeButton_->setVisible(!isSpinnerMode_);
    if (isSpinnerMode_) {
        curSpinnerRotation_ = 0;
        startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, (double)curSpinnerRotation_ / 360.0 * (double)kSpinnerSpeed);
    }

    update();
}

void GeneralMessageWindowItem::onSpinnerRotationChanged(const QVariant &value)
{
    curSpinnerRotation_ = value.toInt();
    update();
}

void GeneralMessageWindowItem::onSpinnerRotationFinished()
{
    curSpinnerRotation_ = 0;
    startAnAnimation<int>(spinnerRotationAnimation_, curSpinnerRotation_, 360, kSpinnerSpeed);
}

void GeneralMessageWindowItem::focusInEvent(QFocusEvent *event)
{
    contentItem_->setFocus();
}

void GeneralMessageWindowItem::updatePositions()
{
    escapeButton_->onLanguageChanged();
    escapeButton_->setPos(WINDOW_WIDTH*G_SCALE - escapeButton_->boundingRect().width() - 16*G_SCALE, 16*G_SCALE);

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH && shape_ != kLoginScreenShape) {
        backArrowButton_->setPos(16*G_SCALE, 12*G_SCALE);
        scrollAreaItem_->setPos(0, 55*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 74*G_SCALE);
    } else {
        backArrowButton_->setPos(16*G_SCALE, 40*G_SCALE);
        scrollAreaItem_->setPos(0, 83*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 102*G_SCALE);
    }
}

void GeneralMessageWindowItem::updateHeight()
{
    int height = 0;

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        height = contentItem_->fullHeight() + 54*G_SCALE;
    } else {
        height = contentItem_->fullHeight() + 82*G_SCALE;
    }

    // minimum height is standard height of connect window
    if (height < WINDOW_HEIGHT) {
        height = WINDOW_HEIGHT;
    }
    setHeight(height);
    emit sizeChanged(this);
}

} // namespace GeneralMessage
