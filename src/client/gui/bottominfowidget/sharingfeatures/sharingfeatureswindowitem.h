#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include <QFont>
#include "sharingfeature.h"
#include "backend/preferences/preferences.h"
#include "commongraphics/dividerline.h"

namespace SharingFeatures {

class SharingFeaturesWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit SharingFeaturesWindowItem(Preferences *preferences, ScalableGraphicsObject * parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject() { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setSecureHotspotFeatures(bool isEnabled, const QString &ssid);
    void setSecureHotspotUsersCount(int usersCount);
    void setProxyGatewayFeatures(bool isEnabled, PROXY_SHARING_TYPE mode);
    void setProxyGatewayUsersCount(int usersCount);

    virtual void animateHornDisplay(bool show);

    void setHorns(bool show);

    virtual int currentHeight() const;

    QPixmap getCurrentPixmapShape();

    QPolygonF leftHornPolygon(int originX, int originY, int width, int height);
    QPolygonF rightHornPolygon(int originX, int originY, int width, int height);

    void updateShareFeatureRoundedness();

    void setClickable(bool enabled);

    void updateScaling() override;

signals:
    void clickedProxy();
    void clickedHotSpot();

private slots:
    void onHornPosChanged(const QVariant &value);
    void onHornOpacityChanged(const QVariant &value);
    void onAppSkinChanged(APP_SKIN s);
    void onLanguageChanged();

private:
    void setHotspotSSID(QString ssid);
    void setProxyType(PROXY_SHARING_TYPE mode);

    enum SHARE_MODE { SHARE_MODE_OFF, SHARE_MODE_HOTSPOT, SHARE_MODE_PROXY, SHARE_MODE_BOTH };

    void updateModedFeatures(SHARE_MODE mode);
    void setMode(SHARE_MODE mode);

    Preferences *preferences_;

    bool isSecureHotspotEnabled_;
    QString secureHotspotSsid_;

    bool isProxyGatewayEnabled_;
    PROXY_SHARING_TYPE proxyGatewayMode_;
    //ProtoTypes::WifiSharingInfo hotspot_;
    //ProtoTypes::ProxySharingInfo gateway_;

    CommonGraphics::DividerLine *dividerLine_;
    CommonGraphics::DividerLine *dividerLine2_;

    QString headerText_;
    SHARE_MODE mode_;

    double curOpacity_;

    static const int WIDTH = 350;
    int height_;

    const char *TEXT_SHARING_FEATURES = QT_TR_NOOP("Sharing Features");
    const char *TEXT_PROXY_GATEWAY = QT_TR_NOOP("Proxy Gateway");
    const char *TEXT_SECURE_HOTSPOT = QT_TR_NOOP("Secure Hotspot");

    void recalcHeight();

    SharingFeature *hotspotFeature_;
    SharingFeature *proxyFeature_;

    bool showingHorns_;

    double curHornOpacity_;
    int curHornPosY_;
    QVariantAnimation hornPosYAnimation_;
    QVariantAnimation hornOpacityAnimation_;

    static const int HEADER_HEIGHT = 48;

    int HORN_POS_Y_HIDDEN;
    int HORN_POS_Y_SHOWING;

    static const int LEFT_HORN_ORIGIN_X = 0;
    static const int HORN_WIDTH = 30;
    static const int HORN_HEIGHT = 30;

    void updatePositions();

};

}
