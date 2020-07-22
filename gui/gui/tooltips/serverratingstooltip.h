#ifndef SERVERRATINGSTOOLTIP_H
#define SERVERRATINGSTOOLTIP_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include "CommonWidgets/iconbuttonwidget.h"
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

private:
    CommonWidgets::IconButtonWidget *rateUpButton_;
    CommonWidgets::IconButtonWidget *rateDownButton_;

    void recalcWidth();
    void recalcHeight();
    void updatePositions();

    const int buttonHeight_ = 16;
    const int buttonWidth_ = 16;
    const int marginHeight_ = 8;
    const int marginWidth_ = 10;
    const int buttonButtonSpacing_ = 10;
    const int textButtonSpacing_ = 10;

    QFont font_;
    ServerRatingState state_;
    QTimer hoverTimer_;

    const QString translatedText();

};

#endif // SERVERRATINGSTOOLTIP_H
