#include "tooltipcontroller.h"

#include <QTimer>
#include "tooltipbasic.h"
#include "tooltipdescriptive.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

#include <QDebug>

TooltipController::TooltipController() : QObject(nullptr)
  , serverRatingsTooltip_(nullptr)
  , serverRatingState_(SERVER_RATING_NONE)
{

}

void TooltipController::hideAllTooltips()
{
    if (serverRatingsTooltip_)
    {
        serverRatingsTooltip_->hide();
    }

    foreach (TooltipId id, tooltips_.keys())
    {
        if (tooltips_.contains(id))
        {
            tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_HIDE);
            tooltips_[id]->hide();
        }
    }
}

void TooltipController::showTooltipInteractive(TooltipId id, int x, int y, int delay)
{
    if (id == TooltipId::TOOLTIP_ID_SERVER_RATINGS)
    {
        if (serverRatingsTooltip_)
        {
            serverRatingsTooltip_->disconnect();
            serverRatingsTooltip_->deleteLater();
            serverRatingsTooltip_ = nullptr;
        }

        serverRatingsTooltip_ = new ServerRatingsTooltip(nullptr);
        serverRatingsTooltip_->setRatingState(serverRatingState_);

        // adjustment to have tail center on x,y //  server rating tooltip has bottom tail
        x -= serverRatingsTooltip_->distanceFromOriginToTailTip();
        y -= serverRatingsTooltip_->getHeight();

        serverRatingsTooltip_->setGeometry(x, y, serverRatingsTooltip_->getWidth(), serverRatingsTooltip_->getHeight());
        connect(serverRatingsTooltip_, SIGNAL(rateUpClicked()), SLOT(onServerRatingsTooltipRateUpClicked()));
        connect(serverRatingsTooltip_, SIGNAL(rateDownClicked()), SLOT(onServerRatingsTooltipRateDownClicked()));

        int actualDelay = TOOLTIP_SHOW_DELAY;
        if (delay != -1) actualDelay = delay;

        QTimer::singleShot(actualDelay, [this](){
            if (serverRatingsTooltip_)
            {
                serverRatingsTooltip_->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                serverRatingsTooltip_->show();
            }
        });
    }
    else
    {
        qCDebug(LOG_BASIC) << "Tooltip ID is not interactive: " << id;
        Q_ASSERT(false);
    }
}

void TooltipController::showTooltipBasic(TooltipInfo info)
{
    TooltipId id = info.id;
    // rebuild each time since setGeometry call (implicit and explicit) doesn't respond well to crossing monitor screens (May be related to QTBUG-63661)
    if (tooltips_.contains(id))
    {
        // do not show an already showing tooltip as it will cause a flicker
        if (tooltips_[id]->getShowState() == TOOLTIP_SHOW_STATE_SHOW && tooltips_[id]->toTooltipInfo() == info)
        {
            return;
        }

        tooltips_[id]->deleteLater();
        tooltips_.remove(id);
    }

    tooltips_[id] = new TooltipBasic(info, nullptr);

    int x = info.x;
    int y = info.y;
    // adjustment to have tail center on x,y
    if (info.tailtype == TOOLTIP_TAIL_LEFT)
    {
        x -= static_cast<TooltipBasic*>(tooltips_[id])->additionalTailWidth();
        y -= tooltips_[id]->distanceFromOriginToTailTip();
    }
    else if (info.tailtype == TOOLTIP_TAIL_BOTTOM)
    {
        x -= tooltips_[id]->distanceFromOriginToTailTip();
        y -= tooltips_[id]->getHeight();
    }
    tooltips_[id]->setGeometry(x, y, tooltips_[id]->getWidth(), tooltips_[id]->getHeight());

    int actualDelay = TOOLTIP_SHOW_DELAY;
    if (info.delay != -1) actualDelay = info.delay;

    QTimer::singleShot(actualDelay, [this, id](){
        if (tooltips_.contains(id))
        {
            if (tooltips_[id]->getShowState() != TOOLTIP_SHOW_STATE_HIDE)
            {
                tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                tooltips_[id]->show();
            }
        }
    });
}

void TooltipController::showTooltipDescriptive(TooltipInfo info)
{
    TooltipId id = info.id;
    if (tooltips_.contains(id))
    {
        tooltips_[id]->deleteLater();
        tooltips_.remove(id);
    }

    tooltips_[id] = new TooltipDescriptive(info, nullptr);

    int x = info.x;
    int y = info.y;
    // adjustment to have tail center on x,y
    if (info.tailtype == TOOLTIP_TAIL_LEFT)
    {
        x -= static_cast<TooltipBasic*>(tooltips_[id])->additionalTailWidth();
        y -= tooltips_[id]->distanceFromOriginToTailTip();
    }
    else if (info.tailtype == TOOLTIP_TAIL_BOTTOM)
    {
        x -= tooltips_[id]->distanceFromOriginToTailTip();
        y -= tooltips_[id]->getHeight();
    }
    tooltips_[id]->setGeometry(x, y, tooltips_[id]->getWidth(), tooltips_[id]->getHeight());

    int actualDelay = TOOLTIP_SHOW_DELAY;
    if (info.delay != -1) actualDelay = info.delay;

    QTimer::singleShot(actualDelay, [this, id](){
        if (tooltips_.contains(id))
        {
            if (tooltips_[id]->getShowState() != TOOLTIP_SHOW_STATE_HIDE)
            {
                tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_SHOW);
                tooltips_[id]->show();
            }
        }
    });

}

void TooltipController::hideTooltip(TooltipId id)
{
    if (id == TooltipId::TOOLTIP_ID_SERVER_RATINGS)
    {
        // give user time to get mouse on the tooltip
        QTimer::singleShot(TOOLTIP_SHOW_DELAY, [this](){
            if (serverRatingsTooltip_)
            {
                if (!serverRatingsTooltip_->isHovering())
                {
                    serverRatingsTooltip_->stopHoverTimer();
                    serverRatingsTooltip_->setShowState(TOOLTIP_SHOW_STATE_HIDE);
                    serverRatingsTooltip_->hide();
                }
                else
                {
                    serverRatingsTooltip_->startHoverTimer();
                }
            }
        });
    }
    else
    {
        if (tooltips_.contains(id))
        {
            tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_HIDE);
            tooltips_[id]->hide();
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

