#include "sharingfeatureswindowitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"

namespace SharingFeatures {

SharingFeaturesWindowItem::SharingFeaturesWindowItem(Preferences *preferences, ScalableGraphicsObject *parent)
  : ScalableGraphicsObject(parent), preferences_(preferences), isSecureHotspotEnabled_(false), isProxyGatewayEnabled_(false),
    proxyGatewayMode_(PROXY_SHARING_HTTP), mode_(SHARE_MODE_OFF), curOpacity_(OPACITY_HIDDEN), height_(0), showingHorns_(false),
    curHornOpacity_(OPACITY_HIDDEN), HORN_POS_Y_HIDDEN(height_ - 30), HORN_POS_Y_SHOWING(height_)
{
    headerText_ = tr(TEXT_SHARING_FEATURES);
    curHornPosY_ = HORN_POS_Y_HIDDEN;

    connect(preferences, &Preferences::appSkinChanged, this, &SharingFeaturesWindowItem::onAppSkinChanged);

    connect(&hornPosYAnimation_, &QVariantAnimation::valueChanged, this, &SharingFeaturesWindowItem::onHornPosChanged);
    connect(&hornOpacityAnimation_, &QVariantAnimation::valueChanged, this, &SharingFeaturesWindowItem::onHornOpacityChanged);

    hotspotFeature_ = new SharingFeature("", "sharingfeatures/SECURE_HOTSPOT_ICON", this);
    proxyFeature_ = new SharingFeature("", "sharingfeatures/COMBINED_SHAPE", this);
    setProxyType(proxyGatewayMode_);

    connect(hotspotFeature_, &SharingFeature::clicked, this, &SharingFeaturesWindowItem::clickedHotSpot);
    connect(proxyFeature_, &SharingFeature::clicked, this, &SharingFeaturesWindowItem::clickedProxy);

    dividerLine_ = new CommonGraphics::DividerLine(this, (WINDOW_WIDTH - 8)*G_SCALE);
    dividerLine_->setOpacity(OPACITY_DIVIDER_LINE_BRIGHT);
    dividerLine2_ = new CommonGraphics::DividerLine(this, (WINDOW_WIDTH - 8)*G_SCALE);
    dividerLine2_->setOpacity(OPACITY_DIVIDER_LINE_BRIGHT);
    updateScaling();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SharingFeaturesWindowItem::onLanguageChanged);
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

void SharingFeaturesWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    qreal initOpacity = painter->opacity();

    painter->setOpacity(curOpacity_ * initOpacity);

    // background
    QRectF background(0, 0, WIDTH*G_SCALE, HEADER_HEIGHT*G_SCALE);
    painter->fillRect(background, FontManager::instance().getMidnightColor());

    const int margin = 16*G_SCALE;

    QFont font = FontManager::instance().getFont(12, QFont::Bold);
    qreal oldLetterSpacing = font.letterSpacing();
    font.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_HALF);

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        painter->drawText(boundingRect().adjusted(16*G_SCALE, 22*G_SCALE, 0, 0), Qt::AlignLeft, headerText_.toUpper().toStdString().c_str());
    }
    else
    {
        int headerTextWidth = CommonGraphics::textWidth(headerText_.toUpper().toStdString().c_str(), font);
        painter->drawText(WIDTH*G_SCALE - headerTextWidth - margin, HEADER_HEIGHT/2*G_SCALE + 10*G_SCALE, headerText_.toUpper().toStdString().c_str());
    }

    font.setLetterSpacing(QFont::AbsoluteSpacing, oldLetterSpacing);

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

void SharingFeaturesWindowItem::setProxyGatewayFeatures(bool isEnabled, PROXY_SHARING_TYPE mode)
{
    if (isProxyGatewayEnabled_ != isEnabled)
    {
        isProxyGatewayEnabled_ = isEnabled;

        SHARE_MODE shareMode = SHARE_MODE_OFF;
        if (isProxyGatewayEnabled_)
        {
            if (isSecureHotspotEnabled_)
            {
                shareMode = SHARE_MODE_BOTH;
            }
            else
            {
                shareMode = SHARE_MODE_PROXY;
            }
        }
        else
        {
            if (isSecureHotspotEnabled_)
            {
                shareMode = SHARE_MODE_HOTSPOT;
            }
        }

        setMode(shareMode);
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
    QString newText = tr(TEXT_SHARING_FEATURES);
    if (mode == SHARE_MODE_PROXY)
    {
        newText  = tr(TEXT_PROXY_GATEWAY);

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
        newText = tr(TEXT_SECURE_HOTSPOT);

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
        newText = tr(TEXT_SHARING_FEATURES);

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

void SharingFeaturesWindowItem::onLanguageChanged()
{
    updateModedFeatures(mode_);
    update();
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

void SharingFeaturesWindowItem::setProxyType(PROXY_SHARING_TYPE mode)
{
    QString proxyText = PROXY_SHARING_TYPE_toString(mode);
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
    dividerLine_->setPos(8*G_SCALE, HEADER_HEIGHT*G_SCALE);
    dividerLine2_->setPos(8*G_SCALE, (HEADER_HEIGHT + SharingFeature::HEIGHT)*G_SCALE);
}

void SharingFeaturesWindowItem::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    update();
}

}
