#include "backgroundimage.h"

#include <QMovie>
#include <QTimer>
#include <spdlog/spdlog.h>

#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"


namespace ConnectWindow {

BackgroundImage::BackgroundImage(QObject *parent, Preferences *preferences) : QObject(parent), preferences_(preferences),
    imageChanger_(this, kAnimationDuration), connectingGradientChanger_(this, kAnimationDuration), isConnected_(false)
{
    // Initialize with the current aspect ratio mode
    imageChanger_.setAspectRatioMode(preferences_->backgroundSettings().aspectRatioMode);
    updateImages();

    connect(&imageChanger_, &ImageChanger::updated, this, &BackgroundImage::updated);
    connect(&connectingGradientChanger_, &SimpleImageChanger::updated, this, &BackgroundImage::updated);
    connect(preferences_, &Preferences::backgroundSettingsChanged, this, &BackgroundImage::onBackgroundSettingsChanged);
    connect(preferences_, &Preferences::appSkinChanged, this, &BackgroundImage::onAppSkinChanged);
    onBackgroundSettingsChanged(preferences_->backgroundSettings());
}

BackgroundImage::~BackgroundImage()
{
}

QPixmap *BackgroundImage::currentPixmap()
{
    return imageChanger_.currentPixmap();
}

QPixmap *BackgroundImage::currentConnectingPixmap()
{
    return connectingGradientChanger_.currentPixmap();
}

void BackgroundImage::changeFlag(const QString &countryCode)
{
    if (isConnected_ && curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS ||
        !isConnected_ && curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS)
    {
        if (countryCode_.isEmpty()) {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                countryCode, kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient();
        }
        else if (countryCode_ != countryCode) {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                countryCode, kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, true);
            switchConnectGradient();
        }
    }
    countryCode_ = countryCode;
}

void BackgroundImage::setIsConnected(bool isConnected)
{
    if (isConnected_ != isConnected) {
        isConnected_ = isConnected;
        handleBackgroundsChange();
    }
}

void BackgroundImage::updateScaling()
{
    handleBackgroundsChange();
}

void BackgroundImage::onBackgroundSettingsChanged(const types::BackgroundSettings &backgroundSettings)
{
    curBackgroundSettings_ = backgroundSettings;
    imageChanger_.setAspectRatioMode(curBackgroundSettings_.aspectRatioMode);
    handleBackgroundsChange();
}

QSharedPointer<QMovie> BackgroundImage::customMovie(const QString &path, bool isConnected)
{
    QSharedPointer<QMovie> movie(new QMovie(path));
    if (!movie->isValid()) {
        return nullptr;
    }

    int height = kDefaultHeight; // ~= 350 * 9 / 16
    if (isConnected && curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_BUNDLED ||
        !isConnected && curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_BUNDLED)
    {
        height = kWidth * 9 / 16;
    } else if (isConnected && curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS ||
               !isConnected && curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS) {
        // Use 2:1 for now for country flags
        height = kFlagHeight;
    } else if (curBackgroundSettings_.aspectRatioMode == ASPECT_RATIO_MODE_FILL) {
        QSize sizeOfImage = QImageReader(isConnected ? curBackgroundSettings_.backgroundImageConnected : curBackgroundSettings_.backgroundImageDisconnected).size();
        height = (double)kWidth / (double)sizeOfImage.width() * (double)sizeOfImage.height();
    }

    if (curBackgroundSettings_.aspectRatioMode != ASPECT_RATIO_MODE_TILE) {
        movie->setScaledSize(QSize(kWidth*G_SCALE, height*G_SCALE) * DpiScaleManager::instance().curDevicePixelRatio());
    }
    return movie;
}

void BackgroundImage::handleBackgroundsChange()
{
    if (isConnected_) {
        if (curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_CUSTOM || curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_BUNDLED) {
            connectedMovie_ = customMovie(curBackgroundSettings_.backgroundImageConnected, true);
            safeChangeToConnectedImage(false);
        } else if (curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_NONE) {
            connectedMovie_.reset();
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                "noflag", kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient();
        } else if (curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS) {
            connectedMovie_.reset();
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                (countryCode_.isEmpty() ? "noflag" : countryCode_), kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient();
        }
    } else {
        if (curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_CUSTOM || curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_BUNDLED) {
            disconnectedMovie_ = customMovie(curBackgroundSettings_.backgroundImageDisconnected, false);
            safeChangeToDisconnectedImage(false);
        } else if (curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_NONE) {
            disconnectedMovie_.reset();
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                "noflag", kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient();
        } else if (curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS) {
            disconnectedMovie_.reset();
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                (countryCode_.isEmpty() ? "noflag" : countryCode_), kWidth*G_SCALE, kFlagHeight*G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient();
        }
    }
}

void BackgroundImage::safeChangeToDisconnectedImage(bool bShowPrevChangeAnimation)
{
    if (disconnectedMovie_) {
        imageChanger_.setMovie(disconnectedMovie_, bShowPrevChangeAnimation);
        switchConnectGradient();
    }
    else {
        imageChanger_.setImage(
            ImageResourcesSvg::instance().getScaledFlag("noflag", kWidth*G_SCALE, kFlagHeight*G_SCALE),
            bShowPrevChangeAnimation);
        switchConnectGradient();
    }
}

void BackgroundImage::safeChangeToConnectedImage(bool bShowPrevChangeAnimation)
{
    if (connectedMovie_) {
        imageChanger_.setMovie(connectedMovie_, bShowPrevChangeAnimation);
        switchConnectGradient();
    }
    else {
        imageChanger_.setImage(
            ImageResourcesSvg::instance().getScaledFlag("noflag", kWidth*G_SCALE, kFlagHeight*G_SCALE),
            bShowPrevChangeAnimation);
        switchConnectGradient();
    }
}

void BackgroundImage::switchConnectGradient()
{
    double opacity = 1.0;

    if (isConnected_ && curBackgroundSettings_.connectedBackgroundType == BACKGROUND_TYPE_CUSTOM ||
        !isConnected_ && curBackgroundSettings_.disconnectedBackgroundType == BACKGROUND_TYPE_CUSTOM)
    {
        opacity = 0.8;
    }

    connectingGradientChanger_.setImage(ImageResourcesSvg::instance().getIndependentPixmap(connectingGradient_), true, opacity);
}

void BackgroundImage::updateImages()
{
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        connectingGradient_ = "background/GRADIENT_BG_CONNECTING_VAN_GOGH";
    } else {
        connectingGradient_ = "background/GRADIENT_BG_CONNECTING";
    }
    switchConnectGradient();
}

void BackgroundImage::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    updateImages();
}

} //namespace ConnectWindow
