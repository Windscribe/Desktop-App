#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include "commonwidgets/iconbuttonwidget.h"
#include "itooltip.h"

enum ServerRatingState { SERVER_RATING_NONE, SERVER_RATING_GOOD, SERVER_RATING_BAD };

class ServerRatingsTooltip : public ITooltip
{
    Q_OBJECT
public:
    explicit ServerRatingsTooltip(QWidget *parent = nullptr);

    void updateScaling() override;

    bool isHovering();
    void setRatingState(ServerRatingState ratingState);
    void startHoverTimer();
    void stopHoverTimer();

    TooltipInfo toTooltipInfo() override;

signals:
    void rateUpClicked();
    void rateDownClicked();

public slots:
    void onRateUpButtonClicked();
    void onRateDownButtonClicked();
    void onHoverTimerTick();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onRateUpButtonHoverEnter();
    void onRateDownButtonHoverEnter();

private:
    CommonWidgets::IconButtonWidget *rateUpButton_;
    CommonWidgets::IconButtonWidget *rateDownButton_;

    void recalcWidth();
    void recalcHeight();
    void updatePositions();

    static constexpr int BUTTON_HEIGHT = 16;
    static constexpr int BUTTON_WIDTH = 16;
    static constexpr int MARGIN_HEIGHT = 8;
    static constexpr int MARGIN_WIDTH = 10;
    static constexpr int BUTTON_BUTTON_SPACING = 10;
    static constexpr int TEXT_BUTTON_SPACING = 10;

    QFont font_;
    ServerRatingState state_;
    QTimer hoverTimer_;

    const QString translatedText();

};
