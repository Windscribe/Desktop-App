#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "backend/preferences/preferences.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/textbutton.h"

class UpdateWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit UpdateWindowItem(Preferences *preferences, ScalableGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setClickable(bool isClickable);

    void startAnimation();
    void stopAnimation();

    void setVersion(QString version, int buildNumber);
    void setProgress(int progressPercent);
    void changeToDownloadingScreen();
    void changeToPromptScreen();
    void setHeight(int height);

signals:
    void acceptClick();
    void cancelClick();
    void laterClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onAcceptClick();
    void onCancelClick();

    void onTitleOpacityChange(const QVariant &value);
    void onLowerTitleOpacityChange(const QVariant &value);
    void onDescriptionOpacityChange(const QVariant &value);
    void onLowerDescriptionOpacityChange(const QVariant &value);

    void onSpinnerOpacityChange(const QVariant &value);
    void onSpinnerRotationChange(const QVariant &value);

    void onUpdatingTimeout();
    void onLanguageChanged();
    void updatePositions();

    void onAppSkinChanged(APP_SKIN s);

private:
    void initScreen();
    Preferences *preferences_;

    CommonGraphics::BubbleButton *acceptButton_;
    CommonGraphics::TextButton *cancelButton_;

    QString curVersion_;
    int curProgress_;
    QString curTitleText_;
    bool downloading_;

    double curTitleOpacity_;
    double curLowerTitleOpacity_;
    double curDescriptionOpacity_;
    double curLowerDescriptionOpacity_;

    QVariantAnimation titleOpacityAnimation_;
    QVariantAnimation lowerTitleOpacityAnimation_;
    QVariantAnimation descriptionOpacityAnimation_;
    QVariantAnimation lowerDescriptionOpacityAnimation_;

    double spinnerOpacity_;
    QVariantAnimation spinnerOpacityAnimation_;
    int spinnerRotation_;
    QVariantAnimation spinnerRotationAnimation_;

    int height_;

    // constants:
    static constexpr int TITLE_POS_Y = 35;

    static constexpr int DESCRIPTION_WIDTH_MIN = 275;
    static constexpr int DESCRIPTION_POS_Y = 86;

    static constexpr int LOWER_DESCRIPTION_WIDTH_MIN = 168;
    static constexpr int LOWER_DESCRIPTION_POS_Y = 156;

    static constexpr int ACCEPT_BUTTON_POS_Y = 164;
    static constexpr int CANCEL_BUTTON_POS_Y = 212;

    static constexpr int SPINNER_ROTATION_TARGET = 360;

    static constexpr int SPINNER_POS_Y = 102;
    static constexpr int SPINNER_HALF_WIDTH = 20;
    static constexpr int SPINNER_HALF_HEIGHT = 20;

    const QString cancelButtonText();
};
