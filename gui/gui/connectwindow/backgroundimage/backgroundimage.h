#ifndef BACKGROUNDIMAGE_H
#define BACKGROUNDIMAGE_H

#include <QObject>
#include <QVariantAnimation>
#include "graphicresources/independentpixmap.h"
#include "../backend/preferences/preferences.h"
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
    QPixmap *currentConnectedPixmap();

    void changeFlag(const QString &countryCode);
    void setIsConnected(bool isConnected);

    void updateScaling();

signals:
    void updated();

private slots:
    void onBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &backgroundSettings);
    void handleBackgroundsChange();

private:
    static constexpr int WIDTH = 332;
    static constexpr int ANIMATION_DURATION = 500;

    QString connectingGradient_;
    QString connectedGradient_;
    QString connectingCustomGradient_;
    QString connectedCustomGradient_;

    Preferences *preferences_;
    ProtoTypes::BackgroundSettings curBackgroundSettings_;
    bool isDisconnectedAndConnectedImagesTheSame_;

    QSharedPointer<QMovie> disconnectedMovie_;
    QSharedPointer<QMovie> connectedMovie_;

    ImageChanger imageChanger_;
    SimpleImageChanger connectingGradientChanger_;
    SimpleImageChanger connectedGradientChanger_;

    QString prevCountryCode_;
    QString countryCode_;
    bool isConnected_;

    void safeChangeToDisconnectedImage(bool bShowPrevChangeAnimation);
    void safeChangeToConnectedImage(bool bShowPrevChangeAnimation);
    void switchConnectGradient(bool isCustomBackground);
};

} //namespace ConnectWindow

#endif // BACKGROUNDIMAGE_H
