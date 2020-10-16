#ifndef UPDATEWINDOWITEM_H
#define UPDATEWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "overlaysconnectwindow/iupdatewindow.h"
#include "commongraphics/bubblebuttonbright.h"
#include "commongraphics/textbutton.h"

namespace UpdateWindow {


class UpdateWindowItem : public ScalableGraphicsObject, public IUpdateWindow
{
    Q_OBJECT
    Q_INTERFACES(IUpdateWindow)
public:
    explicit UpdateWindowItem(ScalableGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override { return this; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void startAnimation() override;
    void stopAnimation() override;
    void updateScaling() override;

    void setVersion(QString version, int buildNumber) override;
    void setProgress(int progressPercent) override;
    void changeToDownloadingScreen() override;
    void changeToPromptScreen() override;

signals:
    void acceptClick() override;
    void cancelClick() override;
    void laterClick() override;

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

private:
    void initScreen();
    CommonGraphics::BubbleButtonBright *acceptButton_;
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

    // constants:
    static constexpr int TITLE_POS_Y = 51;

    static constexpr int DESCRIPTION_WIDTH_MIN = 230;
    static constexpr int DESCRIPTION_POS_Y = 102;

    static constexpr int LOWER_DESCRIPTION_WIDTH_MIN = 168;
    static constexpr int LOWER_DESCRIPTION_POS_Y = 184;

    static constexpr int ACCEPT_BUTTON_POS_Y = 180;
    static constexpr int CANCEL_BUTTON_POS_Y = 236;

    static constexpr int SPINNER_ROTATION_TARGET = 360;

    static constexpr int SPINNER_POS_X_OFFSET = 155;
    static constexpr int SPINNER_POS_Y = 118;
    static constexpr int SPINNER_HALF_WIDTH = 20;
    static constexpr int SPINNER_HALF_HEIGHT = 20;

    const QString cancelButtonText();
};

} // namespace

#endif // UPDATEWINDOWITEM_H
