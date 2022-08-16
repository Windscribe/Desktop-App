#include "backgroundimage.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include <QMovie>
#include <QTimer>


namespace ConnectWindow {

BackgroundImage::BackgroundImage(QObject *parent, Preferences *preferences) : QObject(parent), preferences_(preferences), isDisconnectedAndConnectedImagesTheSame_(false),
    imageChanger_(this, ANIMATION_DURATION), connectingGradientChanger_(this, ANIMATION_DURATION), connectedGradientChanger_(this, ANIMATION_DURATION), isConnected_(false)
{
    updateImages();

    connect(&imageChanger_, &ImageChanger::updated, this, &BackgroundImage::updated);
    connect(&connectingGradientChanger_, &SimpleImageChanger::updated, this, &BackgroundImage::updated);
    connect(&connectedGradientChanger_, &SimpleImageChanger::updated, this, &BackgroundImage::updated);
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

QPixmap *BackgroundImage::currentConnectedPixmap()
{
    return connectedGradientChanger_.currentPixmap();
}

void BackgroundImage::changeFlag(const QString &countryCode)
{
    if (curBackgroundSettings_.backgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS)
    {
        if (countryCode_.isEmpty())
        {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                   countryCode, WIDTH * G_SCALE, 176 * G_SCALE);
            imageChanger_.setImage(indPix, false);
            switchConnectGradient(false);
        }
        else if (countryCode_ != countryCode)
        {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                   countryCode, WIDTH * G_SCALE, 176 * G_SCALE);
            imageChanger_.setImage(indPix, true);
            switchConnectGradient(false);
        }
    }
    countryCode_ = countryCode;
}

void BackgroundImage::setIsConnected(bool isConnected)
{
    if (isConnected_ != isConnected)
    {
        isConnected_ = isConnected;
        if (curBackgroundSettings_.backgroundType == BACKGROUND_TYPE_CUSTOM && !isDisconnectedAndConnectedImagesTheSame_)
        {
            if (isConnected_)
            {
                safeChangeToConnectedImage(true);
            }
            else
            {
                safeChangeToDisconnectedImage(true);
            }
        }
    }
}

void BackgroundImage::updateScaling()
{
    handleBackgroundsChange();
}

void BackgroundImage::onBackgroundSettingsChanged(const types::BackgroundSettings &backgroundSettings)
{
    curBackgroundSettings_ = backgroundSettings;
    handleBackgroundsChange();
}

void BackgroundImage::handleBackgroundsChange()
{
    if (curBackgroundSettings_.backgroundType == BACKGROUND_TYPE_CUSTOM)
    {
        QString disconnectedPath = curBackgroundSettings_.backgroundImageDisconnected;
        QString connectedPath = curBackgroundSettings_.backgroundImageConnected;
        isDisconnectedAndConnectedImagesTheSame_ = (disconnectedPath.compare(connectedPath, Qt::CaseInsensitive) == 0);

        if (disconnectedPath.isEmpty())
        {
            disconnectedMovie_.reset();
        }
        else
        {
            QSharedPointer<QMovie> movie(new QMovie(disconnectedPath));

            if (!movie->isValid())
            {
                disconnectedMovie_.reset();
            }
            else
            {
                movie->setScaledSize(QSize(WIDTH * G_SCALE, 137 * G_SCALE) * DpiScaleManager::instance().curDevicePixelRatio());
                disconnectedMovie_ = movie;
            }
        }

        if (connectedPath.isEmpty())
        {
            connectedMovie_.reset();
        }
        else
        {
            QSharedPointer<QMovie> movie(new QMovie(connectedPath));

            if (!movie->isValid())
            {
                connectedMovie_.reset();
            }
            else
            {
                movie->setScaledSize(QSize(WIDTH * G_SCALE, 137 * G_SCALE) * DpiScaleManager::instance().curDevicePixelRatio());
                connectedMovie_ = movie;
            }
        }

        if (!isDisconnectedAndConnectedImagesTheSame_ && disconnectedMovie_ == nullptr && connectedMovie_ == nullptr)
        {
            isDisconnectedAndConnectedImagesTheSame_ = true;
        }

        if (!isConnected_)
        {
            safeChangeToDisconnectedImage(false);
        }
        else
        {
            safeChangeToConnectedImage(false);
        }
    }
    else if (curBackgroundSettings_.backgroundType == BACKGROUND_TYPE_NONE)
    {
        QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
            "noflag", WIDTH * G_SCALE, 176 * G_SCALE);
        imageChanger_.setImage(indPix, false);
        switchConnectGradient(false);
    }
    else if (curBackgroundSettings_.backgroundType == BACKGROUND_TYPE_COUNTRY_FLAGS)
    {
        QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
            (countryCode_.isEmpty() ? "noflag" : countryCode_), WIDTH * G_SCALE, 176 * G_SCALE);
        imageChanger_.setImage(indPix, false);
        switchConnectGradient(false);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void BackgroundImage::safeChangeToDisconnectedImage(bool bShowPrevChangeAnimation)
{
    if (disconnectedMovie_)
    {
        imageChanger_.setMovie(disconnectedMovie_, bShowPrevChangeAnimation);
        switchConnectGradient(true);
    }
    else
    {
        imageChanger_.setImage(ImageResourcesSvg::instance().getScaledFlag(
                                   "noflag", WIDTH * G_SCALE, 176 * G_SCALE), bShowPrevChangeAnimation);
        switchConnectGradient(false);
    }
}

void BackgroundImage::safeChangeToConnectedImage(bool bShowPrevChangeAnimation)
{
    if (connectedMovie_)
    {
        imageChanger_.setMovie(connectedMovie_, bShowPrevChangeAnimation);
        switchConnectGradient(true);
    }
    else
    {
        imageChanger_.setImage(ImageResourcesSvg::instance().getScaledFlag(
                                   "noflag", WIDTH * G_SCALE, 176 * G_SCALE), bShowPrevChangeAnimation);
        switchConnectGradient(false);
    }
}

void BackgroundImage::switchConnectGradient(bool isCustomBackground)
{
    isCustomBackground_ = isCustomBackground;
    if (isCustomBackground)
    {
        connectingGradientChanger_.setImage(ImageResourcesSvg::instance().getIndependentPixmap(connectingCustomGradient_), true);
        connectedGradientChanger_.setImage(ImageResourcesSvg::instance().getIndependentPixmap(connectedCustomGradient_), true);
    }
    else
    {
        connectingGradientChanger_.setImage(ImageResourcesSvg::instance().getIndependentPixmap(connectingGradient_), true);
        connectedGradientChanger_.setImage(ImageResourcesSvg::instance().getIndependentPixmap(connectedGradient_), true);
    }
}

void BackgroundImage::updateImages()
{
#ifdef Q_OS_MAC
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        connectingGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTING_VAN_GOGH";
        connectedGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTED_VAN_GOGH";
        connectingCustomGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTING_CUSTOM_VAN_GOGH";
        connectedCustomGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTED_CUSTOM_VAN_GOGH";
    }
    else
    {
        connectingGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTING";
        connectedGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTED";
        connectingCustomGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTING_CUSTOM";
        connectedCustomGradient_ = "background/MAC_TOP_GRADIENT_BG_CONNECTED_CUSTOM";
    }
#else
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        connectingGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTING_VAN_GOGH";
        connectedGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTED_VAN_GOGH";
        connectingCustomGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTING_CUSTOM_VAN_GOGH";
        connectedCustomGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTED_CUSTOM_VAN_GOGH";
    }
    else
    {
        connectingGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTING";
        connectedGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTED";
        connectingCustomGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTING_CUSTOM";
        connectedCustomGradient_ = "background/WIN_TOP_GRADIENT_BG_CONNECTED_CUSTOM";
    }
#endif
    switchConnectGradient(isCustomBackground_);
}

void BackgroundImage::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    updateImages();
}

} //namespace ConnectWindow
