#pragma once

#include <QObject>
#include <QVariantAnimation>

#include "backend/preferences/preferences.h"
#include "imagechanger.h"
#include "simpleimagechanger.h"

namespace ConnectWindow {

class BackgroundImage : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundImage(QObject *parent, Preferences *preferences);
    virtual ~BackgroundImage();

    QPixmap *currentPixmap();

    QPixmap *currentConnectingPixmap();

    void changeFlag(const QString &countryCode);
    void setIsConnected(bool isConnected);

    void updateScaling();

signals:
    void updated();

private slots:
    void onBackgroundSettingsChanged(const types::BackgroundSettings &backgroundSettings);
    void handleBackgroundsChange();
    void onAppSkinChanged(APP_SKIN s);

private:
    static constexpr int WIDTH = 350;
    static constexpr int ANIMATION_DURATION = 500;

    QString connectingGradient_;

    Preferences *preferences_;
    types::BackgroundSettings curBackgroundSettings_;

    QSharedPointer<QMovie> disconnectedMovie_;
    QSharedPointer<QMovie> connectedMovie_;

    ImageChanger imageChanger_;
    SimpleImageChanger connectingGradientChanger_;

    QString prevCountryCode_;
    QString countryCode_;
    bool isConnected_;

    void safeChangeToDisconnectedImage(bool bShowPrevChangeAnimation);
    void safeChangeToConnectedImage(bool bShowPrevChangeAnimation);
    void switchConnectGradient();
    void updateImages();
    QSharedPointer<QMovie> customMovie(const QString &path, bool isConnected);
};

} //namespace ConnectWindow
