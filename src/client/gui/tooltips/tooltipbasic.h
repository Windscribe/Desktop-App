#pragma once

#include <QWidget>
#include "tooltiptypes.h"
#include "itooltip.h"

class TooltipBasic : public ITooltip
{
    Q_OBJECT
public:
    explicit TooltipBasic(const TooltipInfo &info, QWidget *parent = nullptr);

    void updateScaling() override;
    TooltipInfo toTooltipInfo() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont font_;
    QString text_;

    static constexpr int MARGIN_WIDTH = 10;
    static constexpr int MARGIN_HEIGHT = 8;

    void recalcWidth();
    void recalcHeight();
};
