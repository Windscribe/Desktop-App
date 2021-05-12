#ifndef BACKGROUNDIMAGE_H
#define BACKGROUNDIMAGE_H

#include <QObject>
#include <QVariantAnimation>
#include "graphicresources/independentpixmap.h"
#include "../backend/preferences/preferences.h"
#include "imagechanger.h"

namespace ConnectWindow {

class BackgroundImage : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundImage(QObject *parent, Preferences *preferences);
    virtual ~BackgroundImage();

    QPixmap *currentPixmap();
    void changeFlag(const QString &countryCode);
    void setIsConnected(bool isConnected);

signals:
    void updated();

private slots:
    void onBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &backgroundSettings);
    void handleBackgroundsChange();

private:
    static constexpr int WIDTH = 332;

    Preferences *preferences_;
    ProtoTypes::BackgroundSettings curBackgroundSettings_;

    QSharedPointer<IndependentPixmap> disconnectedImage_;
    QSharedPointer<IndependentPixmap> connectedImage_;

    ImageChanger imageChanger_;

    QString prevCountryCode_;
    QString countryCode_;
    bool isConnected_;


    QSharedPointer<IndependentPixmap> safeGetDisconnectedImage();
    QSharedPointer<IndependentPixmap> safeGetConnectedImage();
};

} //namespace ConnectWindow

#endif // BACKGROUNDIMAGE_H
