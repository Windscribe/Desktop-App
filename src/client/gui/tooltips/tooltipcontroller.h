#pragma once

#include <QObject>
#include "serverratingstooltip.h"
#include "tooltiptypes.h"
#include "itooltip.h"

class TooltipController : public QObject
{
    Q_OBJECT
public:
    static TooltipController &instance()
    {
        static TooltipController t;
        return t;
    }

    void hideAllTooltips();
    void showTooltipInteractive(TooltipId id, int x, int y, int delay, QWidget *parent = nullptr);
    void showTooltipBasic(TooltipInfo info);
    void showTooltipDescriptive(TooltipInfo info);
    void hideTooltip(TooltipId type);

    void clearServerRatings();

signals:
    void sendServerRatingUp();
    void sendServerRatingDown();

private slots:
    void onServerRatingsTooltipRateUpClicked();
    void onServerRatingsTooltipRateDownClicked();
    void onServerRatingHideTimerTimeout();

private:
    TooltipController();

    QTimer serverRatingsHideTimer_;
    ServerRatingsTooltip *serverRatingsTooltip_;
    ServerRatingState serverRatingState_;

    QMap<TooltipId, ITooltip*> tooltips_;

};
