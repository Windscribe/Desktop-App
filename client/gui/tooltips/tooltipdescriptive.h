#pragma once

#include <QWidget>
#include <QLabel>
#include "tooltiptypes.h"
#include "itooltip.h"

class TooltipDescriptive : public ITooltip
{
    Q_OBJECT
public:
    explicit TooltipDescriptive(const TooltipInfo &info, QWidget *parent = nullptr);

    void updateScaling() override;
    TooltipInfo toTooltipInfo() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont fontTitle_;
    QFont fontDescr_;
    QString textTitle_;
    QLabel labelDescr_;

    void recalcHeight();

    static constexpr int MARGIN_WIDTH = 6;
    static constexpr int MARGIN_HEIGHT = 8;
    static constexpr int TITLE_DESC_SPACING = 10;

    int leftTooltipMinY() const;
    int leftTooltipMaxY() const;
    int bottomTooltipMaxX() const;
    int bottomTooltipMinX() const;

    int widthOfDescriptionLabel() const;

};
