#include "tooltipcontroller.h"

#include <QTimer>
#include "tooltipbasic.h"
#include "tooltipdescriptive.h"

TooltipController::TooltipController() : QObject(nullptr)
{
}

void TooltipController::hideAllTooltips()
{
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
    if (tooltips_.contains(id)) {
        tooltips_[id]->setShowState(TOOLTIP_SHOW_STATE_HIDE);
        if (tooltips_[id]->getAnimate()) {
            tooltips_[id]->startFadeOutAnimation(tooltips_[id]->getAnimationSpeed());
        } else {
            tooltips_[id]->hide();
        }
    }
}

