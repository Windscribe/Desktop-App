#ifndef TOOLTIPDESCRIPTIVE_H
#define TOOLTIPDESCRIPTIVE_H

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

    const int MARGIN_WIDTH = 6;
    const int MARGIN_HEIGHT = 8;
    const int TITLE_DESC_SPACING = 10;

    int leftTooltipMinY() const;
    int leftTooltipMaxY() const;
    int bottomTooltipMaxX() const;
    int bottomTooltipMinX() const;

    int widthOfDescriptionLabel() const;

};

#endif // TOOLTIPDESCRIPTIVE_H
