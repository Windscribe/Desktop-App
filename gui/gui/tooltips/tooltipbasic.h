#ifndef TOOLTIPBASIC_H
#define TOOLTIPBASIC_H

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

    const int marginWidth_ = 10;
    const int marginHeight_ = 8;

    void recalcWidth();
    void recalcHeight();

};

#endif // TOOLTIPBASIC_H
