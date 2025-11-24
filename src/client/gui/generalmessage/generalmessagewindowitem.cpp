#include "generalmessagewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GeneralMessageWindow {

GeneralMessageWindowItem::GeneralMessageWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper,
        GeneralMessageWindow::Style style, const QString &icon, const QString &title, const QString &desc,
        const QString &acceptText, const QString &rejectText, const QString &tertiaryText) :
    ResizableWindow(parent, preferences, preferencesHelper), isSpinnerMode_(false), shape_(GeneralMessageWindow::kLoginScreenShape)
{
    setBackButtonEnabled(false);
    setResizeBarEnabled(false);

    contentItem_ = new GeneralMessageItem(this, WINDOW_WIDTH, style);
    contentItem_->setIcon(icon);
    contentItem_->setTitle(title);
    contentItem_->setDescription(desc);
    contentItem_->setAcceptText(acceptText);
    contentItem_->setTertiaryText(tertiaryText);
    contentItem_->setRejectText(rejectText);
    scrollAreaItem_->setItem(contentItem_);

    setVanGoghOffset(31);

    // Do not go into Van Gogh mode if at login screen
    if (shape_ == GeneralMessageWindow::kLoginScreenShape) {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_BOTTOM);
    }

    connect(contentItem_, &GeneralMessageItem::acceptClick, this, &GeneralMessageWindowItem::acceptClick);
    connect(contentItem_, &GeneralMessageItem::rejectClick, this, &GeneralMessageWindowItem::rejectClick);
    connect(contentItem_, &GeneralMessageItem::tertiaryClick, this, &GeneralMessageWindowItem::tertiaryClick);
    connect(&spinnerRotationAnimation_, &QVariantAnimation::valueChanged, this, &GeneralMessageWindowItem::onSpinnerRotationChanged);
    connect(&spinnerRotationAnimation_, &QVariantAnimation::finished, this, &GeneralMessageWindowItem::onSpinnerRotationFinished);
    connect(this, &ResizableWindow::escape, this, &GeneralMessageWindowItem::onEscape);

    updateHeight();
}

void GeneralMessageWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // bottom-most background: draw this first since the base background has a border
    painter->setPen(QColor(9, 15, 25));
    painter->setBrush(QColor(9, 15, 25));
    painter->drawRoundedRect(getBottomResizeArea(), 9*G_SCALE, 9*G_SCALE);
    painter->fillRect(getBottomResizeArea().adjusted(0, -2*G_SCALE, 0, -9*G_SCALE), QBrush(QColor(9, 15, 25)));

    // base background
    if (shape_ == GeneralMessageWindow::kConnectScreenAlphaShape) {
        painter->fillRect(boundingRect().adjusted(0, 32*G_SCALE, 0, -9*G_SCALE), QBrush(QColor(9, 15, 25)));

        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
    } else {
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
    }

    if (isSpinnerMode_) {
        int offset = 0;
        painter->save();
        painter->setPen(QPen(Qt::white, 2 * G_SCALE));
        painter->translate(boundingRect().width()/2, boundingRect().height()/2 + offset);
        painter->rotate(curSpinnerRotation_);
        const int circleDiameter = 80*G_SCALE;
        painter->drawArc(QRect(-circleDiameter/2, -circleDiameter/2, circleDiameter, circleDiameter), 0, 4 * 360);
        painter->restore();
    }

    if (shape_ == GeneralMessageWindow::kConnectScreenAlphaShape) {
        QSharedPointer<IndependentPixmap> pixmapBorder = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorder_);
        pixmapBorder->draw(0, 0, 350*G_SCALE, 32*G_SCALE, painter);
    } else {
        QSharedPointer<IndependentPixmap> pixmapBorder = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_TOP_INNER_VAN_GOGH");
        pixmapBorder->draw(0, 0, 350*G_SCALE, 32*G_SCALE, painter);
    }

    QSharedPointer<IndependentPixmap> pixmapBorderExtension = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorderExtension_);
    pixmapBorderExtension->draw(0, 32*G_SCALE, 350*G_SCALE, boundingRect().height() - 64*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> pixmapBorderBottom = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorderBottom_);
    pixmapBorderBottom->draw(0, curHeight_ - 32*G_SCALE, 350*G_SCALE, 32*G_SCALE, painter);
}

void GeneralMessageWindowItem::setStyle(GeneralMessageWindow::Style style)
{
    contentItem_->setStyle(style);
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

void GeneralMessageWindowItem::setAcceptText(const QString &text, bool showRemember)
{
    contentItem_->setAcceptText(text, showRemember);
    updateHeight();
}

void GeneralMessageWindowItem::setRejectText(const QString &text)
{
    contentItem_->setRejectText(text);
    updateHeight();
}

void GeneralMessageWindowItem::setTertiaryText(const QString &text)
{
    contentItem_->setTertiaryText(text);
    updateHeight();
}

void GeneralMessageWindowItem::setTitleSize(int size)
{
    contentItem_->setTitleSize(size);
    updateHeight();
}

void GeneralMessageWindowItem::setBackgroundShape(GeneralMessageWindow::Shape shape)
{
    shape_ = shape;
    updatePositions();
}

void GeneralMessageWindowItem::setShowBottomPanel(bool on)
{
    contentItem_->setShowBottomPanel(on);
    updateHeight();
}

void GeneralMessageWindowItem::setLearnMoreUrl(const QString &url)
{
    contentItem_->setLearnMoreUrl(url);
}

bool GeneralMessageWindowItem::isRememberChecked()
{
    return contentItem_->isRememberChecked();
}

void GeneralMessageWindowItem::onEscape()
{
    if (!isSpinnerMode_) {
        emit rejectClick();
    }
}

void GeneralMessageWindowItem::onAppSkinChanged(APP_SKIN s)
{
    ResizableWindow::onAppSkinChanged(s);
    contentItem_->update();
    updateHeight();
    update();
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

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        if (shape_ != GeneralMessageWindow::kLoginScreenShape) {
            scrollAreaItem_->setPos(0, 39*G_SCALE);
            scrollAreaItem_->setHeight(curHeight_ - 39*G_SCALE);
        } else {
            scrollAreaItem_->setPos(0, 83*G_SCALE);
            scrollAreaItem_->setHeight(curHeight_ - 83*G_SCALE);
        }
    } else {
        if (shape_ != GeneralMessageWindow::kLoginScreenShape) {
            scrollAreaItem_->setPos(0, 67*G_SCALE);
            scrollAreaItem_->setHeight(curHeight_ - 67*G_SCALE);
        } else {
            scrollAreaItem_->setPos(0, 83*G_SCALE);
            scrollAreaItem_->setHeight(curHeight_ - 83*G_SCALE);
        }
    }

    updateHeight();
}

void GeneralMessageWindowItem::updateHeight()
{
    int height = 0;

    if (shape_ == GeneralMessageWindow::kLoginScreenShape) {
        height = contentItem_->fullHeight() + 98*G_SCALE;

        if (height < LOGIN_HEIGHT*G_SCALE) {
            height = LOGIN_HEIGHT*G_SCALE;
        }
    } else {
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
            height = contentItem_->fullHeight() + 40*G_SCALE;
            if (height < WINDOW_HEIGHT_VAN_GOGH*G_SCALE) {
                height = WINDOW_HEIGHT_VAN_GOGH*G_SCALE;
            }
        } else {
            height = contentItem_->fullHeight() + 70*G_SCALE;
            if (height < WINDOW_HEIGHT*G_SCALE) {
                height = WINDOW_HEIGHT*G_SCALE;
            }
        }
    }

    if (curHeight_ != height) {
        setHeight(height);
        emit sizeChanged(this);
    }

    update();
    contentItem_->update();
}

GeneralMessageWindow::Shape GeneralMessageWindowItem::backgroundShape() const
{
    return shape_;
}

void GeneralMessageWindowItem::setShowUsername(bool on)
{
    contentItem_->setShowUsername(on);
}

void GeneralMessageWindowItem::setShowPassword(bool on)
{
    contentItem_->setShowPassword(on);
}

QString GeneralMessageWindowItem::username() const
{
    return contentItem_->username();
}

QString GeneralMessageWindowItem::password() const
{
    return contentItem_->password();
}

void GeneralMessageWindowItem::clear()
{
    contentItem_->clear();
}

void GeneralMessageWindowItem::setUsername(const QString &username)
{
    contentItem_->setUsername(username);
}

} // namespace GeneralMessage
