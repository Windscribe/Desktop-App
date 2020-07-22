#ifndef UPDATEWINDOWITEM_H
#define UPDATEWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "Update/iupdatewindow.h"
#include "CommonGraphics/bubblebuttonbright.h"
#include "CommonGraphics/textbutton.h"

namespace UpdateWindow {


class UpdateWindowItem : public ScalableGraphicsObject, public IUpdateWindow
{
    Q_OBJECT
    Q_INTERFACES(IUpdateWindow)
public:
    explicit UpdateWindowItem(bool upgrade, ScalableGraphicsObject *parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject() { return this; }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void startAnimation();
    void stopAnimation();

    void setVersion(QString version);
    void setProgress(int progressPercent);
    void changeToDownloadingScreen();

signals:
    void acceptClick();
    void cancelClick();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void onAcceptClick();
    void onCancelClick();

    void onBackgroundOpacityChange(const QVariant &value);
    void onTitleOpacityChange(const QVariant &value);
    void onLowerTitleOpacityChange(const QVariant &value);
    void onDescriptionOpacityChange(const QVariant &value);
    void onLowerDescriptionOpacityChange(const QVariant &value);

    void onSpinnerOpacityChange(const QVariant &value);
    void onSpinnerRotationChange(const QVariant &value);

    void onUpdatingTimeout();

    void onLanguageChanged();

private:
    void initScreen();
    bool upgradeMode_;

    CommonGraphics::BubbleButtonBright *acceptButton_;
    CommonGraphics::TextButton *cancelButton_;

    QString curVersion_;
    int curProgress_;

    QString curTitleText_;
    QString curLowerTitleText_;
    QString curDescriptionText_;
    QString curLowerDescriptionText_;

    double curBackgroundOpacity_;
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

    QColor titleColor_;

    QString background_;

    // constants:
    const int TITLE_POS_Y = 51;

    const int DESCRIPTION_WIDTH_MIN = 230;
    const int DESCRIPTION_POS_Y = 102;

    const int LOWER_DESCRIPTION_WIDTH_MIN = 168;
    const int LOWER_DESCRIPTION_POS_Y = 184;

    const int ACCEPT_BUTTON_POS_Y = 180;
    const int CANCEL_BUTTON_POS_Y = 236;

    const int SPINNER_ROTATION_TARGET = 360;

    const int SPINNER_POS_X_OFFSET = 155;
    const int SPINNER_POS_Y = 118;
    const int SPINNER_HALF_WIDTH = 20;
    const int SPINNER_HALF_HEIGHT = 20;
};

} // namespace

#endif // UPDATEWINDOWITEM_H
