#include "protocolwindowitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"
#include "../../common/types/protocolstatus.h"

namespace ProtocolWindow {

ProtocolWindowItem::ProtocolWindowItem(QGraphicsObject *parent,
                                       ConnectWindow::ConnectWindowItem *connectWindow,
                                       Preferences *preferences,
                                       PreferencesHelper *preferencesHelper)
    : ResizableWindow(parent, preferences, preferencesHelper),
      protocolPromptItem_(new ProtocolPromptItem(this, connectWindow, preferences, preferencesHelper, ProtocolWindowMode::kChangeProtocol)),
      promptHeight_(protocolPromptItem_->currentHeight())
{
    WS_ASSERT(preferencesHelper);

    setBackButtonEnabled(false);

    scrollAreaItem_->setItem(protocolPromptItem_);

    connect(escapeButton_, &CommonGraphics::EscapeButton::clicked, this, &ProtocolWindowItem::onEscape);
    connect(protocolPromptItem_, &ProtocolPromptItem::escape, this, &ProtocolWindowItem::escape);
    connect(protocolPromptItem_, &ProtocolPromptItem::protocolClicked, this, &ProtocolWindowItem::protocolClicked);
    connect(protocolPromptItem_, &ProtocolPromptItem::heightChanged, this, &ProtocolWindowItem::onPromptHeightChanged);
    connect(protocolPromptItem_, &ProtocolPromptItem::stopConnection, this, &ProtocolWindowItem::stopConnection);

    onPromptHeightChanged(promptHeight_);
}

void ProtocolWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // resize area background
    painter->fillRect(boundingRect().adjusted(0, 32*G_SCALE, 0, -9*G_SCALE), QBrush(QColor(9, 15, 25)));

    // base background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
    } else {
        QSharedPointer<IndependentPixmap> pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
        pixmapBaseBackground->draw(0, 0, painter);
    }

    QSharedPointer<IndependentPixmap> pixmapBorder = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorder_);
    pixmapBorder->draw(0, 0, 350*G_SCALE, 32*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> pixmapBorderExtension = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBorderExtension_);
    pixmapBorderExtension->draw(0, 32*G_SCALE, 350*G_SCALE, getBottomResizeArea().toRect().top() - 32*G_SCALE, painter);

    // bottom-most background
    if (roundedFooter_) {
        painter->setPen(footerColor_);
        painter->setBrush(footerColor_);
        painter->drawRoundedRect(getBottomResizeArea(), 9*G_SCALE, 9*G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2*G_SCALE, 0, -9*G_SCALE), QBrush(footerColor_));
    } else {
        painter->fillRect(getBottomResizeArea(), QBrush(footerColor_));
    }
}

void ProtocolWindowItem::setMode(ProtocolWindowMode mode)
{
    protocolPromptItem_->setMode(mode);
}

void ProtocolWindowItem::setProtocolStatus(const types::ProtocolStatus &status)
{
    protocolPromptItem_->setProtocolStatus(status);
}

void ProtocolWindowItem::setProtocolStatus(const QVector<types::ProtocolStatus> &status)
{
    bool hasNonFailedProtocol = false;

    for (auto s : status) {
        if (s.status != types::ProtocolStatus::Status::kFailed) {
            hasNonFailedProtocol = true;
            break;
        }
    }

    if (hasNonFailedProtocol) {
        protocolPromptItem_->setMode(ProtocolWindowMode::kFailedProtocol);
        protocolPromptItem_->setProtocolStatus(status);
        allProtocolsFailed_ = false;
    } else {
        allProtocolsFailed_ = true;
    }
}

void ProtocolWindowItem::onPromptHeightChanged(int height)
{
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        setHeight(height + 74*G_SCALE);
    } else {
        setHeight(height + 102*G_SCALE);
    }
    promptHeight_ = height;
    emit sizeChanged(this);
}

void ProtocolWindowItem::onAppSkinChanged(APP_SKIN skin)
{
    ResizableWindow::onAppSkinChanged(skin);
    onPromptHeightChanged(promptHeight_);
}

void ProtocolWindowItem::resetProtocolStatus()
{
    allProtocolsFailed_ = false;
    protocolPromptItem_->resetProtocolStatus();
}

void ProtocolWindowItem::onEscape()
{
    if (protocolPromptItem_->mode() == ProtocolWindowMode::kFailedProtocol) {
        emit stopConnection();
    }
    emit escape();
}

bool ProtocolWindowItem::hasMoreAttempts()
{
    return !allProtocolsFailed_;
}

} // namespace ProtocolWindow
