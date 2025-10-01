#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "backend/preferences/preferences.h"
#include "commongraphics/bubblebutton.h"

namespace UpdateApp {

class UpdateAppItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    enum UpdateAppItemMode { UPDATE_APP_ITEM_MODE_PROMPT, UPDATE_APP_ITEM_MODE_PROGRESS };

    explicit UpdateAppItem(Preferences *preferences, QGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setVersionAvailable(const QString &versionNumber, int buildNumber);
    void setProgress(int value); // from 0 to 100
    QPixmap getCurrentPixmapShape();

    void setMode(UpdateAppItemMode mode);

signals:
    void updateClick();

private slots:
    void onBackgroundOpacityChanged(const QVariant &value);
    void onVersionOpacityChanged(const QVariant &value);
    void onProgressForegroundOpacityChanged(const QVariant &value);
    void onProgressBackgroundOpacityChanged(const QVariant &value);
    void onProgressBarPosChanged(const QVariant &value);
    void onAppSkinChanged(APP_SKIN s);

    void onUpdateClick();

    void onLanguageChanged();

private:
    void animateTransitionToProgress();
    void animateTransitionToVersion();

    Preferences *preferences_;

    CommonGraphics::BubbleButton *updateButton_;
    UpdateAppItemMode mode_;
    QString curVersionText_;

    static constexpr int WIDTH = 248;
    static constexpr int WIDTH_VAN_GOGH = 350;
    static constexpr int HEIGHT = 20;

    double curBackgroundOpacity_;
    double curVersionOpacity_;

    double curProgressBackgroundOpacity_;
    double curProgressForegroundOpacity_;

    QVariantAnimation backgroundOpacityAnimation_;
    QVariantAnimation versionOpacityAnimation_;
    QVariantAnimation progressForegroundOpacityAnimation_;
    QVariantAnimation progressBackgroundOpacityAnimation_;

    int curProgressBarPos_;  //  0 -> 100
    QVariantAnimation progressBarPosChangeAnimation_;

    static constexpr int VERSION_TEXT_HEIGHT = 14;

    void updatePositions();
};

} // namespace UpdateApp
