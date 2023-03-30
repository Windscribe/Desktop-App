#include "protocolwindow/protocolpromptitem.h"

#include <QDesktopServices>
#include <QPainter>
#include <QUrl>

#include "dpiscalemanager.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/scrollarea.h"
#include "graphicresources/fontmanager.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"

namespace ProtocolWindow {

ProtocolPromptItem::ProtocolPromptItem(ScalableGraphicsObject *parent,
                                       IConnectWindow *connectWindow,
                                       Preferences *preferences,
                                       PreferencesHelper *preferencesHelper,
                                       ProtocolWindowMode mode)
    : BasePage(parent, WINDOW_WIDTH), connectWindow_(connectWindow), preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(kSpacerHeight);
    setIndent(kIndent);
    resetProtocolStatus();
    setMode(mode);

    connect(preferencesHelper_, &PreferencesHelper::portMapChanged, this, &ProtocolPromptItem::resetProtocolStatus);

    countdownTimer_.setInterval(1000);
    connect(&countdownTimer_, &QTimer::timeout, this, &ProtocolPromptItem::onTimerTimeout);
}

void ProtocolPromptItem::paint(QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/)
{
    // title
    painter->setOpacity(OPACITY_FULL);
    QFont *titleFont = FontManager::instance().getFont(16, true);
    painter->setFont(*titleFont);
    painter->setPen(QColor(Qt::white));

    // 60 for icon + 24 spacing
    painter->drawText(boundingRect().adjusted(36*G_SCALE, 84*G_SCALE, -36*G_SCALE, 0),
                      Qt::AlignHCenter | Qt::TextWordWrap, title_);

    // description
    painter->setOpacity(OPACITY_HALF);
    QFont *descFont = FontManager::instance().getFont(14, false);
    painter->setFont(*descFont);

    // 60 for icon + 24 spacing + title + 16 spacing
    painter->drawText(boundingRect().adjusted(36*G_SCALE, titleHeight_ + 100*G_SCALE, -36*G_SCALE, 0),
                      Qt::AlignHCenter | Qt::TextWordWrap, desc_);
}

void ProtocolPromptItem::clearCountdown()
{
    for (auto i : items()) {
        if (ProtocolLineItem *item = dynamic_cast<ProtocolLineItem *>(i)) {
            item->clearCountdown();
        }
    }
}

void ProtocolPromptItem::resetProtocolStatus()
{
    resetProtocolStatus_ = true;
}

void ProtocolPromptItem::doResetProtocolStatus()
{
    // if reset flag not set and we have existing statuses, use existing statuses
    if (!resetProtocolStatus_ && !statuses_.empty()) {
        setProtocolStatus(statuses_);
        return;
    }

    clearItems();
    statuses_.clear();

    // Put the protocol shown on connect screen first, regardless if it's connected
    types::ProtocolStatus currentProtocol = connectWindow_->getProtocolStatus();
    ProtocolLineItem *item = new ProtocolLineItem(this, currentProtocol, getProtocolDescription(currentProtocol.protocol));
    connect(item, &ProtocolLineItem::clicked, this, &ProtocolPromptItem::onProtocolClicked);
    statuses_ << currentProtocol;
    items_ << item;
    addItem(item);

    for (types::Protocol &p : preferencesHelper_->getAvailableProtocols()) {
        // already added this at the beginning, skip it
        if (currentProtocol.protocol == p) {
            continue;
        }

        QVector<uint> ports = preferencesHelper_->getAvailablePortsForProtocol(p);
        if (ports.size() == 0) {
            continue;
        }
        
        types::ProtocolStatus ps = types::ProtocolStatus(p, ports[0], types::ProtocolStatus::Status::kDisconnected, -1);
        statuses_ << ps;

        ProtocolLineItem *item = new ProtocolLineItem(this, ps, getProtocolDescription(p));
        connect(item, &ProtocolLineItem::clicked, this, &ProtocolPromptItem::onProtocolClicked);
        items_ << item;
        addItem(item);
    }

    addCancelButton();

    resetProtocolStatus_ = false;
}

void ProtocolPromptItem::setProtocolStatus(const types::ProtocolStatus &ps)
{
    QVector<types::ProtocolStatus> statuses;

    for (auto status : statuses_) {
        if (status.protocol == ps.protocol && status.port == ps.port) {
            status.status = ps.status;
        }

        if (status.status == types::ProtocolStatus::Status::kConnected) {
            statuses.prepend(status);
        } else {
            statuses << status;
        }
    }

    setProtocolStatus(statuses);
}

void ProtocolPromptItem::setProtocolStatus(const QVector<types::ProtocolStatus> &statuses)
{
    clearItems();

    statuses_ = statuses;

    for (auto status : statuses) {
        ProtocolLineItem *item = new ProtocolLineItem(this, status, getProtocolDescription(status.protocol));
        connect(item, &ProtocolLineItem::clicked, this, &ProtocolPromptItem::onProtocolClicked);
        items_ << item;
        addItem(item);
        if (!countdownTimer_.isActive() && status.timeout > 0) {
            countdownTimer_.start();
        }
    }

    addCancelButton();
}

ProtocolWindowMode ProtocolPromptItem::mode()
{
    return mode_;
}

void ProtocolPromptItem::setMode(ProtocolWindowMode mode)
{
    if (mode == ProtocolWindowMode::kUninitialized) {
        return;
    } else if (mode == ProtocolWindowMode::kChangeProtocol) {
        setIcon("CHANGE_PROTOCOL");
        title_ = tr(kChangeProtocolTitle);
        desc_ = tr(kChangeProtocolDescription);
        doResetProtocolStatus();
    } else if (mode == ProtocolWindowMode::kFailedProtocol) {
        setIcon("WARNING");
        title_ = tr(kFailedProtocolTitle);
        desc_ = tr(kFailedProtocolDescription);
        doResetProtocolStatus();
    } else if (mode == ProtocolWindowMode::kSavePreferredProtocol) {
        setIcon("WARNING");
        title_ = QString(tr(kSavePreferredProtocolTitle)).arg(connectedProtocol().toLongString());
        desc_ = tr(kSavePreferredProtocolDescription);
        clearItems();

        actionButton_ = new ProtocolListButton(this, kSetAsPreferredButton, false, QColor(0, 0, 0));
        connect(actionButton_, &ProtocolListButton::clicked, this, &ProtocolPromptItem::onSetAsPreferred);
        addItem(actionButton_);
        addCancelButton();
    } else if (mode == ProtocolWindowMode::kSendDebugLog) {
        resetProtocolStatus();

        setIcon("WARNING");
        title_ = tr(kSendDebugLogTitle);
        desc_ = tr(kSendDebugLogDescription);
        clearItems();

        actionButton_ = new ProtocolListButton(this, kSendDebugLogButton, false, QColor(255, 255, 255));
        connect(actionButton_, &ProtocolListButton::clicked, this, &ProtocolPromptItem::onSendDebugLog);
        addItem(actionButton_);
        addCancelButton();
    } else if (mode == ProtocolWindowMode::kDebugLogSent) {
        setIcon("CHECKMARK_IN_CIRCLE");
        title_ = tr(kDebugLogSentTitle);
        desc_ = tr(kDebugLogSentDescription);
        clearItems();

        actionButton_ = new ProtocolListButton(this, kContactSupportButton, false, QColor(255, 255, 255));
        connect(actionButton_, &ProtocolListButton::clicked, this, &ProtocolPromptItem::onContactSupport);
        addItem(actionButton_);
        addCancelButton();
    }
    mode_ = mode;
    updateItemOffset();
    update();
    emit heightChanged(fullHeight());
}

void ProtocolPromptItem::onSetAsPreferred()
{
    countdownTimer_.stop();
    emit escape();
    emit setAsPreferredProtocol(types::ConnectionSettings(lastProtocol_, lastPort_, false));
}

void ProtocolPromptItem::onSendDebugLog()
{
    emit sendDebugLog();
    setMode(ProtocolWindowMode::kDebugLogSent);
}

void ProtocolPromptItem::onContactSupport()
{
    countdownTimer_.stop();
    emit escape();
    QDesktopServices::openUrl(QUrl(QString("https://%1/support/ticket").arg(HardcodedSettings::instance().serverUrl())));
}

void ProtocolPromptItem::addCancelButton()
{
    cancelButton_ = new ProtocolListButton(this, kCancelButton, true);
    connect(cancelButton_, &ProtocolListButton::clicked, this, &ProtocolPromptItem::onCancelClicked);
    addItem(cancelButton_);
}

void ProtocolPromptItem::setIcon(const QString &path)
{
    icon_.reset(new ImageItem(this, path, ""));
    icon_->setPos((WINDOW_WIDTH/2 - 30)*G_SCALE, 0);
}

QString ProtocolPromptItem::getProtocolDescription(const types::Protocol &protocol)
{
    if (protocol == types::Protocol::TYPE::WIREGUARD) {
        return tr("Cutting-edge protocol.");
    } else if (protocol == types::Protocol::TYPE::IKEV2) {
        return tr("An IPsec based tunneling protocol.");
    } else if (protocol == types::Protocol::TYPE::OPENVPN_UDP) {
        return tr("Balanced speed and security.");
    } else if (protocol == types::Protocol::TYPE::OPENVPN_TCP) {
        return tr("Use it if OpenVPN UDP fails.");
    } else if (protocol == types::Protocol::TYPE::STUNNEL) {
        return tr("Disguises traffic as HTTPS with TLS.");
    } else if (protocol == types::Protocol::TYPE::WSTUNNEL) {
        return tr("Wraps traffic with web sockets.");
    } else {
        WS_ASSERT(false);
        return tr("");
    }
}

void ProtocolPromptItem::onProtocolClicked()
{
    ProtocolLineItem *item = qobject_cast<ProtocolLineItem*>(sender());
    lastProtocol_ = item->protocol();
    lastPort_ = item->port();
    emit escape();
    emit protocolClicked(item->protocol(), item->port());
}

void ProtocolPromptItem::updateItemOffset()
{
    // Calculate how far down we should start drawing protocols/buttons
    QFont *titleFont = FontManager::instance().getFont(16, true);
    QFontMetrics titleMetrics(*titleFont);
    titleHeight_ = titleMetrics.boundingRect(boundingRect().adjusted(36*G_SCALE, 0, -36*G_SCALE, 0).toRect(),
                                             Qt::AlignHCenter | Qt::TextWordWrap, title_).height();
    QFont *descFont = FontManager::instance().getFont(14, false);
    QFontMetrics descMetrics(*descFont);
    descHeight_ = descMetrics.boundingRect(boundingRect().adjusted(36*G_SCALE, 0, -36*G_SCALE, 0).toRect(),
                                           Qt::AlignHCenter | Qt::TextWordWrap, desc_).height();

    // 60 for icon + 24 spacing + title + 16 spacing + description + 24 spacing
    setFirstItemOffsetY((titleHeight_ + descHeight_ + 124*G_SCALE)/G_SCALE);
}

void ProtocolPromptItem::onTimerTimeout()
{
    for (auto i : items()) {
        if (ProtocolLineItem *item = dynamic_cast<ProtocolLineItem *>(i)) {
            int secs = item->decrementCountdown();
            if (secs == 0) {
                emit escape();
                countdownTimer_.stop();
                return;
            }
        }
    }
    countdownTimer_.start();
}

void ProtocolPromptItem::onCancelClicked()
{
    if (mode_ == ProtocolWindowMode::kFailedProtocol) {
        emit stopConnection();
    }
    emit escape();
}

void ProtocolPromptItem::updateScaling()
{
    BasePage::updateScaling();
    for (auto i : items()) {
        i->updateScaling();
    }
    setMode(mode_);
}

types::Protocol ProtocolPromptItem::connectedProtocol() {
    for (auto i : items()) {
        if (ProtocolLineItem *item = dynamic_cast<ProtocolLineItem *>(i)) {
            if (item->status() == types::ProtocolStatus::Status::kConnected) {
                return item->protocol();
            }
        }
    }
    return types::Protocol(types::Protocol::TYPE::UNINITIALIZED);
}

} // namespace ProtocolWindow
