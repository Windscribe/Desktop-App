#ifndef ITOOLTIP_H
#define ITOOLTIP_H

#include <QWidget>
#include "tooltiptypes.h"

enum TooltipShowState {
    TOOLTIP_SHOW_STATE_INIT,
    TOOLTIP_SHOW_STATE_SHOW,
    TOOLTIP_SHOW_STATE_HIDE
};

class ITooltip : public QWidget
{
public:
    ITooltip(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~ITooltip() {}

    int getWidth() const;
    int getHeight() const;

    TooltipShowState getShowState();
    void setShowState(TooltipShowState showState);

    int distanceFromOriginToTailTip() const;
    int additionalTailWidth() const;
    int additionalTailHeight() const;

    QPolygonF trianglePolygonLeft(int tailLeftPtX, int tailLeftPtY);
    QPolygonF trianglePolygonBottom(int tailLeftPtX, int tailLeftPtY);
    void triangleLeftPointXY(int &tailLeftPtX, int &tailLeftPtY);

    virtual void updateScaling() = 0;
    virtual TooltipInfo toTooltipInfo() = 0;

protected:
    TooltipId id_;
    int width_; // scaled
    int height_; // scaled
    TooltipTailType tailType_;
    double tailPosPercent_;
    TooltipShowState showState_;

    void initWindowFlags();
    int leftTooltipMinY() const;
    int leftTooltipMaxY() const;
    int bottomTooltipMaxX() const;
    int bottomTooltipMinX() const;

};

Q_DECLARE_INTERFACE(ITooltip, "ITooltip")

#endif // ITOOLTIP_H
