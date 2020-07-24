#include "sharingfeatureswindowitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/protoenumtostring.h"
#include <google/protobuf/util/message_differencer.h>
#include "dpiscalemanager.h"

namespace SharingFeatures {

SharingFeaturesWindowItem::SharingFeaturesWindowItem(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    isSecureHotspotEnabled_(false), isProxyGatewayEnabled_(false),
    proxyGatewayMode_(ProtoTypes::PROXY_SHARING_HTTP)
{
    headerText_ = TEXT_SHARING_FEATURES;

    showingHorns_ = false;

    height_ = 0;

    HORN_POS_Y_HIDDEN = height_ - 30;
    HORN_POS_Y_SHOWING = height_;

    curHornPosY_ = HORN_POS_Y_HIDDEN;

    curHornOpacity_ = OPACITY_HIDDEN;

    mode_ = SHARE_MODE_OFF;

    connect(&hornPosYAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onHornPosChanged(QVariant)));
    connect(&hornOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onHornOpacityChanged(QVariant)));

    curOpacity_ = OPACITY_HIDDEN;

    hotspotFeature_ = new SharingFeature("", "sharingfeatures/SECURE_HOTSPOT_ICON", this);
    proxyFeature_ = new SharingFeature("", "sharingfeatures/COMBINED_SHAPE", this);
    setProxyType(proxyGatewayMode_);

    connect(hotspotFeature_, SIGNAL(clicked()), SIGNAL(clickedHotSpot()));
    connect(proxyFeature_, SIGNAL(clicked()), SIGNAL(clickedProxy()));

    dividerLine_ = new PreferencesWindow::DividerLine(this, WIDTH);
    dividerLine2_ = new PreferencesWindow::DividerLine(this, WIDTH);
    updateScaling();
}

QRectF SharingFeaturesWindowItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, height_*G_SCALE);
}

QPolygonF SharingFeaturesWindowItem::leftHornPolygon(int originX, int originY, int width, int height)
{
    QPainterPath path;
    path.moveTo(originX, originY);
    path.arcTo(originX, originY, width, height, 90, 90); // arc to bottom left
    path.lineTo(originX + width/2, originY);            // to right pt
    path.lineTo(originX, originY + height/2);           // bot pt
    path.closeSubpath();

    QPolygonF poly = path.toFillPolygon();
    return poly;
}

QPolygonF SharingFeaturesWindowItem::rightHornPolygon(int originX, int originY, int width, int height)
{
    QPainterPath path;
    path.moveTo(originX + width, originY);     // top right
    path.arcTo(originX, originY, width, height, 90, -90);  // arc to bottom left
    path.closeSubpath();

    QPolygonF poly = path.toFillPolygon();
    return poly;
}

void SharingFeaturesWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    qreal initOpacity = painter->opacity();

    painter->setOpacity(curOpacity_ * initOpacity);

    // background
    QRectF background(0, 0, WIDTH*G_SCALE, HEADER_HEIGHT*G_SCALE);
    painter->fillRect(background, FontManager::instance().getMidnightColor());

    const int margin = 16*G_SCALE;

    // header text
    QFont *font = FontManager::instance().getFont(16, true);
    painter->setFont(*font);
    painter->setPen(Qt::white);

    int headerTextWidth = CommonGraphics::textWidth(tr(headerText_.toStdString().c_str()), *font);
    painter->drawText(WIDTH*G_SCALE - headerTextWidth - margin, HEADER_HEIGHT/2*G_SCALE + CommonGraphics::textHeight(*font)/4, tr(headerText_.toStdString().c_str()));

    // horns:
    painter->setOpacity(curHornOpacity_ * initOpacity);
    painter->setPen(Qt::transparent);
    painter->setBrush(FontManager::instance().getMidnightColor());

    // Left horn
    QPolygonF poly = leftHornPolygon(LEFT_HORN_ORIGIN_X*G_SCALE, curHornPosY_*G_SCALE, HORN_WIDTH*G_SCALE, HORN_HEIGHT*G_SCALE);
    painter->drawPolygon(poly);

    // Right horn
    QPolygonF poly2 = rightHornPolygon( (WIDTH - HORN_WIDTH)*G_SCALE, curHornPosY_*G_SCALE, HORN_WIDTH*G_SCALE, HORN_HEIGHT*G_SCALE);
    painter->drawPolygon(poly2);
}

void SharingFeaturesWindowItem::setSecureHotspotFeatures(bool isEnabled, const QString &ssid)
{
    if (isSecureHotspotEnabled_ != isEnabled)
    {
        isSecureHotspotEnabled_ = isEnabled;

        SHARE_MODE mode = SHARE_MODE_OFF;
        if (isSecureHotspotEnabled_)
        {
            if (isProxyGatewayEnabled_)
            {
                mode = SHARE_MODE_BOTH;
            }
            else
            {
                mode = SHARE_MODE_HOTSPOT;
            }
        }
        else
        {
            if (isProxyGatewayEnabled_)
            {
                mode = SHARE_MODE_PROXY;
            }
        }

        setMode(mode);
    }

    if (secureHotspotSsid_ != ssid)
    {
        secureHotspotSsid_ = ssid;
        setHotspotSSID(ssid);
    }
}

void SharingFeaturesWindowItem::setSecureHotspotUsersCount(int usersCount)
{
    hotspotFeature_->setNumber(usersCount);
}

void SharingFeaturesWindowItem::setProxyGatewayFeatures(bool isEnabled, ProtoTypes::ProxySharingMode mode)
{
    if (isProxyGatewayEnabled_ != isEnabled)
    {
        isProxyGatewayEnabled_ = isEnabled;

        SHARE_MODE mode = SHARE_MODE_OFF;
        if (isProxyGatewayEnabled_)
        {
            if (isSecureHotspotEnabled_)
            {
                mode = SHARE_MODE_BOTH;
            }
            else
            {
                 mode = SHARE_MODE_PROXY;
            }
        }
        else
        {
            if (isSecureHotspotEnabled_)
            {
                mode = SHARE_MODE_HOTSPOT;
            }
        }

        setMode(mode);
    }

    if (proxyGatewayMode_ != mode)
    {
        proxyGatewayMode_ = mode;
        setProxyType(proxyGatewayMode_);
    }
}

void SharingFeaturesWindowItem::setProxyGatewayUsersCount(int usersCount)
{
    proxyFeature_->setNumber(usersCount);
}

void SharingFeaturesWindowItem::updateModedFeatures(SHARE_MODE mode)
{
    QString newText = TEXT_SHARING_FEATURES;
    if (mode == SHARE_MODE_PROXY)
    {
        newText  = QT_TR_NOOP("Proxy Gateway");

        curOpacity_ = OPACITY_FULL;

        dividerLine2_->hide();

        proxyFeature_->setClickable(true);
        proxyFeature_->setPos(0, HEADER_HEIGHT*G_SCALE);
        proxyFeature_->setOpacityByFactor(OPACITY_FULL);

        hotspotFeature_->setPos(0, -HEADER_HEIGHT*G_SCALE); // so doesn't interfere with visible button
        hotspotFeature_->setClickable(false);
        hotspotFeature_->setOpacityByFactor(OPACITY_HIDDEN);
    }
    else if (mode == SHARE_MODE_HOTSPOT)
    {
        newText = QT_TR_NOOP("Secure Hotspot");

        curOpacity_ = OPACITY_FULL;

        dividerLine2_->hide();

        hotspotFeature_->setClickable(true);
        hotspotFeature_->setPos(0, HEADER_HEIGHT*G_SCALE);
        hotspotFeature_->setOpacityByFactor(OPACITY_FULL);

        proxyFeature_->setPos(0, -HEADER_HEIGHT*G_SCALE); // so doesn't interfere with visible button
        proxyFeature_->setClickable(false);
        proxyFeature_->setOpacityByFactor(OPACITY_HIDDEN);
    }
    else if (mode == SHARE_MODE_BOTH)
    {
        newText = TEXT_SHARING_FEATURES;

        curOpacity_ = OPACITY_FULL;

        dividerLine2_->show();

        hotspotFeature_->setPos(0, HEADER_HEIGHT*G_SCALE);
        hotspotFeature_->setClickable(true);

        proxyFeature_->setPos(0, (HEADER_HEIGHT + SharingFeature::HEIGHT)*G_SCALE);
        proxyFeature_->setClickable(true);

        hotspotFeature_->setOpacityByFactor(OPACITY_FULL);
        proxyFeature_->setOpacityByFactor(OPACITY_FULL);
    }
    else
    {
        newText = "";

        curOpacity_ = OPACITY_HIDDEN;

        dividerLine2_->hide();

        hotspotFeature_->setClickable(false);
        proxyFeature_->setClickable(false);

        hotspotFeature_->setOpacityByFactor(OPACITY_HIDDEN);
        proxyFeature_->setOpacityByFactor(OPACITY_HIDDEN);
    }

    headerText_ = newText;
}

void SharingFeaturesWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    updateModedFeatures(mode_);
    updateShareFeatureRoundedness();
    recalcHeight();
    updatePositions();
}

void SharingFeaturesWindowItem::setMode(SHARE_MODE mode)
{
    if (mode != mode_)
    {
        mode_ = mode;

        updateModedFeatures(mode);
        updateShareFeatureRoundedness();

        recalcHeight();
        update();
    }

}

void SharingFeaturesWindowItem::setHotspotSSID(QString ssid)
{
    hotspotFeature_->setText(ssid);
}

void SharingFeaturesWindowItem::setProxyType(ProtoTypes::ProxySharingMode mode)
{
    QString proxyText = ProtoEnumToString::instance().toString(mode);
    proxyFeature_->setText(proxyText);
}

void SharingFeaturesWindowItem::updateShareFeatureRoundedness()
{
#ifdef Q_OS_WIN
    hotspotFeature_->setRounded(false);
    proxyFeature_->setRounded(false);
#else
    if (showingHorns_)
    {
        hotspotFeature_->setRounded(false);
        proxyFeature_->setRounded(false);
    }
    else
    {
        if (mode_ == SHARE_MODE_PROXY)
        {
            proxyFeature_->setRounded(true);

        }
        else if (mode_ == SHARE_MODE_HOTSPOT)
        {
            hotspotFeature_->setRounded(true);

        }
        else if (mode_ == SHARE_MODE_BOTH)
        {
            hotspotFeature_->setRounded(false);
            proxyFeature_->setRounded(true);

        }
        else
        {
            hotspotFeature_->setRounded(false);
            proxyFeature_->setRounded(false);
        }
    }
#endif
}

void SharingFeaturesWindowItem::setClickable(bool enabled)
{
    if (enabled)
    {
        updateModedFeatures(mode_); // for setClickable
    }
    else
    {
        hotspotFeature_->setClickable(enabled);
        proxyFeature_->setClickable(enabled);
    }
}

void SharingFeaturesWindowItem::animateHornDisplay(bool show)
{
    showingHorns_ = show;

    int nextHornPosY = HORN_POS_Y_HIDDEN;
    double nextHornOpacity = OPACITY_HIDDEN;
    if (show)
    {
        nextHornPosY = HORN_POS_Y_SHOWING;
        nextHornOpacity = OPACITY_FULL;
    }

    startAnAnimation<int>(hornPosYAnimation_, curHornPosY_, nextHornPosY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(hornOpacityAnimation_, curHornOpacity_, nextHornOpacity, ANIMATION_SPEED_FAST);
}

void SharingFeaturesWindowItem::setHorns(bool show)
{
    showingHorns_ = show;

    int nextHornPosY = HORN_POS_Y_HIDDEN;
    double nextHornOpacity = OPACITY_HIDDEN;
    if (show)
    {
        nextHornPosY = HORN_POS_Y_SHOWING;
        nextHornOpacity = OPACITY_FULL;
    }

    updateShareFeatureRoundedness();

    curHornPosY_ = nextHornPosY;
    curHornOpacity_ = nextHornOpacity;
    update();
}

int SharingFeaturesWindowItem::currentHeight() const
{
    return height_*G_SCALE;
}

QPixmap SharingFeaturesWindowItem::getCurrentPixmapShape()
{
    int height = height_*G_SCALE;
    if (showingHorns_) height += HORN_HEIGHT*G_SCALE;

    QPixmap tempPixmap(WIDTH*G_SCALE, height);
    tempPixmap.fill(Qt::transparent);

    QPainter painter(&tempPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(FontManager::instance().getMidnightColor());
    painter.setPen(Qt::transparent);
    {   
        painter.drawRect(0, 0, WIDTH*G_SCALE, height);

        QPolygonF leftPoly = leftHornPolygon(LEFT_HORN_ORIGIN_X*G_SCALE, curHornPosY_*G_SCALE, HORN_WIDTH*G_SCALE, HORN_HEIGHT*G_SCALE);
        QPolygonF rightPoly = rightHornPolygon( (WIDTH - HORN_WIDTH)*G_SCALE, curHornPosY_*G_SCALE, HORN_WIDTH*G_SCALE, HORN_HEIGHT*G_SCALE);

        painter.drawPolygon(leftPoly);
        painter.drawPolygon(rightPoly);
    }

    return tempPixmap;
}

void SharingFeaturesWindowItem::onHornPosChanged(const QVariant &value)
{
    curHornPosY_ = value.toInt();
    update();
}

void SharingFeaturesWindowItem::onHornOpacityChanged(const QVariant &value)
{
    curHornOpacity_ = value.toDouble();
    update();
}

void SharingFeaturesWindowItem::recalcHeight()
{
    int newHeight = 0;
    if (mode_ == SHARE_MODE_HOTSPOT)    newHeight = HEADER_HEIGHT + SharingFeature::HEIGHT;
    else if (mode_ == SHARE_MODE_PROXY) newHeight = HEADER_HEIGHT + SharingFeature::HEIGHT;
    else if (mode_ == SHARE_MODE_BOTH)  newHeight = HEADER_HEIGHT + SharingFeature::HEIGHT*2;

	prepareGeometryChange();
    height_ = newHeight;
    HORN_POS_Y_HIDDEN = height_ - 30;
    HORN_POS_Y_SHOWING = height_;
}

void SharingFeaturesWindowItem::updatePositions()
{
    dividerLine_->setPos(24*G_SCALE, HEADER_HEIGHT*G_SCALE);
    dividerLine2_->setPos(24*G_SCALE, (HEADER_HEIGHT + SharingFeature::HEIGHT)*G_SCALE);
}

}
