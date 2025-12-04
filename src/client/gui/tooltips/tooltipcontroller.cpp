#include "tooltipcontroller.h"

#include <QTimer>
#include "tooltipbasic.h"
#include "tooltipdescriptive.h"
#include "dpiscalemanager.h"
#include "utils/ws_assert.h"
#include "utils/log/logger.h"

#include <QDebug>

TooltipController::TooltipController() : QObject(nullptr)
  , serverRatingsTooltip_(nullptr)
  , serverRatingState_(SERVER_RATING_NONE)
{
    serverRatingsHideTimer_.setInterval(TOOLTIP_SHOW_DELAY);
    serverRatingsHideTimer_.setSingleShot(true);
    connect(&serverRatingsHideTimer_, &QTimer::timeout, this, &TooltipController::onServerRatingHideTimerTimeout);
}

void TooltipController::hideAllTooltips()
{
    if (serverRatingsTooltip_) {
        // when server rating tooltip steals activation/focus prevent it from hiding itself
        if (!serverRatingsTooltip_->isHovering()) {
            serverRatingsTooltip_->hide();
        }
    }

    const auto tooltipsKeys = tooltips_.keys();
    for (TooltipId id : tooltipsKeys) {
        if (tooltips_.contains(id)) {
            tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_HIDE);
            if (tooltips_[id]->getAnimate()) {
                tooltips_[id]->startFadeOutAnimation(tooltips_[id]->getAnimationSpeed());
            } else {
                tooltips_[id]->hide();
            }
        }
    }
}

void TooltipController::showTooltipInteractive(TooltipId id, int x, int y, int delay, QWidget *parent)
{
    if (id == TooltipId::TOOLTIP_ID_SERVER_RATINGS) {
        serverRatingsHideTimer_.stop();
        if (serverRatingsTooltip_) {
            serverRatingsTooltip_->disconnect();
            serverRatingsTooltip_->deleteLater();
            serverRatingsTooltip_ = nullptr;
        }

#if defined(Q_OS_LINUX)
        serverRatingsTooltip_ = new ServerRatingsTooltip(parent);
#else
        serverRatingsTooltip_ = new ServerRatingsTooltip(nullptr);
#endif
        serverRatingsTooltip_->setRatingState(serverRatingState_);

        // adjustment to have tail center on x,y //  server rating tooltip has bottom tail
        x -= serverRatingsTooltip_->distanceFromOriginToTailTip();
        y -= serverRatingsTooltip_->getHeight();

        serverRatingsTooltip_->setGeometry(x, y, serverRatingsTooltip_->getWidth(), serverRatingsTooltip_->getHeight());
        connect(serverRatingsTooltip_, &ServerRatingsTooltip::rateUpClicked, this, &TooltipController::onServerRatingsTooltipRateUpClicked);
        connect(serverRatingsTooltip_, &ServerRatingsTooltip::rateDownClicked, this, &TooltipController::onServerRatingsTooltipRateDownClicked);

        int actualDelay = TOOLTIP_SHOW_DELAY;
        if (delay != -1) actualDelay = delay;

        QTimer::singleShot(actualDelay, [this](){
            if (serverRatingsTooltip_) {
                serverRatingsTooltip_->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                serverRatingsTooltip_->show();
            }
        });
    } else {
        qCCritical(LOG_BASIC) << "Tooltip ID is not interactive: " << id;
        WS_ASSERT(false);
    }
}

void TooltipController::showTooltipBasic(TooltipInfo info)
{
    TooltipId id = info.id;
    // rebuild each time since setGeometry call (implicit and explicit) doesn't respond well to crossing monitor screens (May be related to QTBUG-63661)
    if (tooltips_.contains(id)) {
        // do not show an already showing tooltip as it will cause a flicker
        if (tooltips_[id]->getShowState() == TOOLTIP_SHOW_STATE_SHOW && tooltips_[id]->toTooltipInfo() == info) {
            return;
        }

        tooltips_[id]->deleteLater();
        tooltips_.remove(id);
    }

#if defined(Q_OS_LINUX)
    tooltips_[id] = new TooltipBasic(info, info.parent);
#else
    tooltips_[id] = new TooltipBasic(info, nullptr);
#endif

    int x = info.x;
    int y = info.y;
    // adjustment to have tail center on x,y
    if (info.tailtype == TOOLTIP_TAIL_LEFT) {
        x -= static_cast<TooltipBasic*>(tooltips_[id])->additionalTailWidth();
        y -= tooltips_[id]->distanceFromOriginToTailTip();
    } else if (info.tailtype == TOOLTIP_TAIL_BOTTOM) {
        x -= tooltips_[id]->distanceFromOriginToTailTip();
        y -= tooltips_[id]->getHeight();
    }
    tooltips_[id]->setGeometry(x, y, tooltips_[id]->getWidth(), tooltips_[id]->getHeight());

    int actualDelay = TOOLTIP_SHOW_DELAY;
    if (info.delay != -1) actualDelay = info.delay;

    QTimer::singleShot(actualDelay, [this, id, info]() {
        if (tooltips_.contains(id)) {
            if (tooltips_[id]->getShowState() != TOOLTIP_SHOW_STATE_HIDE) {
                tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                if (info.animate) {
                    tooltips_[id]->startFadeInAnimation(info.animationSpeed);
                } else {
                    tooltips_[id]->show();
                }
            }
        }
    });
}

void TooltipController::showTooltipDescriptive(TooltipInfo info)
{
    TooltipId id = info.id;
    if (tooltips_.contains(id)) {
        tooltips_[id]->deleteLater();
        tooltips_.remove(id);
    }

#if defined(Q_OS_LINUX)
    tooltips_[id] = new TooltipDescriptive(info, info.parent);
#else
    tooltips_[id] = new TooltipDescriptive(info, nullptr);
#endif

    int x = info.x;
    int y = info.y;
    // adjustment to have tail center on x,y
    if (info.tailtype == TOOLTIP_TAIL_LEFT) {
        x -= static_cast<TooltipBasic*>(tooltips_[id])->additionalTailWidth();
        y -= tooltips_[id]->distanceFromOriginToTailTip();
    } else if (info.tailtype == TOOLTIP_TAIL_BOTTOM) {
        x -= tooltips_[id]->distanceFromOriginToTailTip();
        y -= tooltips_[id]->getHeight();
    }
    tooltips_[id]->setGeometry(x, y, tooltips_[id]->getWidth(), tooltips_[id]->getHeight());

    int actualDelay = TOOLTIP_SHOW_DELAY;
    if (info.delay != -1) actualDelay = info.delay;

    QTimer::singleShot(actualDelay, [this, id, info]() {
        if (tooltips_.contains(id)) {
            if (tooltips_[id]->getShowState() != TOOLTIP_SHOW_STATE_HIDE) {
                tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                if (info.animate) {
                    tooltips_[id]->startFadeInAnimation(info.animationSpeed);
                } else {
                    tooltips_[id]->show();
                }
            }
        }
    });
}

void TooltipController::hideTooltip(TooltipId id)
{
    if (id == TooltipId::TOOLTIP_ID_SERVER_RATINGS) {
        // give user time to get mouse on the tooltip
        serverRatingsHideTimer_.start();
    } else {
        if (tooltips_.contains(id)) {
            tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_HIDE);
            if (tooltips_[id]->getAnimate()) {
                tooltips_[id]->startFadeOutAnimation(tooltips_[id]->getAnimationSpeed());
            } else {
                tooltips_[id]->hide();
            }
        }
    }
}

void TooltipController::clearServerRatings()
{
    serverRatingState_ = SERVER_RATING_NONE;
}

void TooltipController::onServerRatingsTooltipRateUpClicked()
{
    serverRatingState_ = SERVER_RATING_GOOD;
    emit sendServerRatingUp();
}

void TooltipController::onServerRatingsTooltipRateDownClicked()
{
    serverRatingState_ = SERVER_RATING_BAD;
    emit sendServerRatingDown();
}

void TooltipController::onServerRatingHideTimerTimeout()
{
    if (serverRatingsTooltip_) {
        if (!serverRatingsTooltip_->isHovering()) {
            serverRatingsTooltip_->stopHoverTimer();
            serverRatingsTooltip_->setShowState(TOOLTIP_SHOW_STATE_HIDE);
            serverRatingsTooltip_->hide();
        } else {
            serverRatingsTooltip_->startHoverTimer();
        }
    }
}

