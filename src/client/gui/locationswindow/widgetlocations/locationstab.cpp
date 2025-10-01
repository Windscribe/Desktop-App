#include "locationstab.h"

#include <QPainter>
#include <QtMath>

#include "backend/persistentstate.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/ws_assert.h"

extern QWidget *g_mainWindow;

namespace GuiLocations {

LocationsTab::LocationsTab(QWidget *parent, Preferences *preferences, gui_locations::LocationsModelManager *locationsModelManager) : QWidget(parent)
  , preferences_(preferences)
  , locationsModelManager_(locationsModelManager)
  , countOfVisibleItemSlots_(7)
  , currentLocationListHeight_(0)
  , isRibbonVisible_(false)
  , showAllTabs_(true)
  , curTab_(LOCATION_TAB_ALL_LOCATIONS)
  , isUnlimitedData_(true) // Assume true until told otherwise, so we make sure not to show the banner unless user is confirmed to be non-premium
  , connectState_(CONNECT_STATE_DISCONNECTED)

{
    connect(preferences_, &Preferences::appSkinChanged, this, &LocationsTab::onAppSkinChanged);

    // all locations
    gui_locations::LocationsView *viewAllLocations = new gui_locations::LocationsView(this, locationsModelManager->sortedLocationsProxyModel());
    connect(viewAllLocations, &gui_locations::LocationsView::selected, this, &LocationsTab::onLocationSelected);
    connect(viewAllLocations, &gui_locations::LocationsView::clickedOnPremiumStarCity, this, &LocationsTab::onClickedOnPremiumStarCity);
    EmptyListWidget *emptyListWidgetAllLocations = new EmptyListWidget(this);
    widgetAllLocations_ = new WidgetSwitcher(this, viewAllLocations, emptyListWidgetAllLocations);

    // custom configs
    gui_locations::LocationsView * viewConfiguredLocations = new gui_locations::LocationsView(this, locationsModelManager->customConfigsProxyModel());
    connect(viewConfiguredLocations, &gui_locations::LocationsView::selected, this, &LocationsTab::onLocationSelected);
    connect(viewConfiguredLocations, &gui_locations::LocationsView::clickedOnPremiumStarCity, this, &LocationsTab::onClickedOnPremiumStarCity);
    EmptyListWidget *emptyListWidgetConfigured = new EmptyListWidget(this);
    emptyListWidgetConfigured->setIcon("locations/FOLDER_ICON_BIG");
    connect(emptyListWidgetConfigured, &EmptyListWidget::clicked, [this]() {
        emit addCustomConfigClicked();
    });
    widgetConfiguredLocations_ = new WidgetSwitcher(this, viewConfiguredLocations, emptyListWidgetConfigured);
    widgetConfiguredLocations_->hide();

    // static IPs
    gui_locations::LocationsView *viewStaticIpsLocations_ = new gui_locations::LocationsView(this, locationsModelManager->staticIpsProxyModel());
    connect(viewStaticIpsLocations_, &gui_locations::LocationsView::selected, this, &LocationsTab::onLocationSelected);
    connect(viewStaticIpsLocations_, &gui_locations::LocationsView::clickedOnPremiumStarCity, this, &LocationsTab::onClickedOnPremiumStarCity);
    EmptyListWidget *emptyListWidgetStaticIps = new EmptyListWidget(this);
    emptyListWidgetStaticIps->setIcon("locations/STATIC_IP_ICON_BIG");
    emptyListWidgetStaticIps->hide();
    connect(emptyListWidgetStaticIps, &EmptyListWidget::clicked, [this]() {
        emit addStaticIpClicked();
    });
    widgetStaticIpsLocations_ = new WidgetSwitcher(this, viewStaticIpsLocations_, emptyListWidgetStaticIps);
    widgetStaticIpsLocations_->hide();
    connect(widgetStaticIpsLocations_, &WidgetSwitcher::widgetsSwicthed, [this]() {
        updateRibbonVisibility();
    });

    // favorites
    gui_locations::LocationsView *viewFavoriteLocations_ = new gui_locations::LocationsView(this, locationsModelManager->favoriteCitiesProxyModel());
    viewFavoriteLocations_->setShowCountryFlagForCity(true);
    connect(viewFavoriteLocations_, &gui_locations::LocationsView::selected, this, &LocationsTab::onLocationSelected);
    connect(viewFavoriteLocations_, &gui_locations::LocationsView::clickedOnPremiumStarCity, this, &LocationsTab::onClickedOnPremiumStarCity);
    EmptyListWidget *emptyListWidgetFavorites = new EmptyListWidget(this);
    emptyListWidgetFavorites->setIcon("locations/BROKEN_HEART_ICON");
    widgetFavoriteLocations_ = new WidgetSwitcher(this, viewFavoriteLocations_, emptyListWidgetFavorites);
    widgetFavoriteLocations_->hide();

    // search locations
    gui_locations::LocationsView *viewSearchLocations = new gui_locations::LocationsView(this, locationsModelManager->filterLocationsProxyModel());
    connect(viewSearchLocations, &gui_locations::LocationsView::selected, this, &LocationsTab::onLocationSelected);
    connect(viewSearchLocations, &gui_locations::LocationsView::clickedOnPremiumStarCity, this, &LocationsTab::onClickedOnPremiumStarCity);
    EmptyListWidget *emptyListWidgetSearchLocations = new EmptyListWidget(this);
    widgetSearchLocations_ = new WidgetSwitcher(this, viewSearchLocations, emptyListWidgetSearchLocations);
    widgetSearchLocations_->hide();

    staticIPDeviceInfo_ = new StaticIPDeviceInfo(this);
    connect(staticIPDeviceInfo_, &StaticIPDeviceInfo::addStaticIpClicked, this, &LocationsTab::addStaticIpClicked);
    staticIPDeviceInfo_->hide();

    configFooterInfo_ = new ConfigFooterInfo(this);
    configFooterInfo_->hide();

    connect(configFooterInfo_, &ConfigFooterInfo::clearCustomConfigClicked, this, &LocationsTab::clearCustomConfigClicked);
    connect(configFooterInfo_, &ConfigFooterInfo::addCustomConfigClicked, this, &LocationsTab::addCustomConfigClicked);

    upgradeBanner_ = new UpgradeBanner(this);
    upgradeBanner_->hide();
    connect(upgradeBanner_, &UpgradeBanner::clicked, this, &LocationsTab::upgradeBannerClicked);

    refreshButtonText_ = "";
    refreshButtonColor_ = QColor(0x5D, 0x87, 0xD0);
    refreshButtonAnimationProgress_ = 0.0;
    pingsActive_ = false;
    currentSpinnerRotation_ = 0.0;

    refreshButtonColorAnimation_ = new QVariantAnimation(this);
    refreshButtonColorAnimation_->setDuration(ANIMATION_SPEED_FAST);
    connect(refreshButtonColorAnimation_, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        refreshButtonAnimationProgress_ = value.toDouble();
        update();
    });

    spinnerRotationAnimation_ = new QVariantAnimation(this);
    spinnerRotationAnimation_->setStartValue(0.0);
    spinnerRotationAnimation_->setEndValue(360.0);
    spinnerRotationAnimation_->setDuration(1000); // 1 second per rotation
    spinnerRotationAnimation_->setLoopCount(-1); // Infinite loop
    connect(spinnerRotationAnimation_, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        currentSpinnerRotation_ = value.toDouble();
        update();
    });

    setMouseTracking(true);

    updateLocationWidgetsGeometry(unscaledHeightOfItemViewport());

    connect(locationsModelManager, &gui_locations::LocationsModelManager::deviceNameChanged, this, &LocationsTab::onDeviceNameChanged);
    updateCustomConfigsEmptyListVisibility();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LocationsTab::onLanguageChanged);
    onLanguageChanged();

    onClickAllLocations();
}

void LocationsTab::setCountVisibleItemSlots(int cnt)
{
    if (cnt != countOfVisibleItemSlots_)
    {
        countOfVisibleItemSlots_ = cnt;
        updateRibbonVisibility();
        updateLocationWidgetsGeometry(unscaledHeightOfItemViewport());
    }
}

int LocationsTab::getCountVisibleItems()
{
    return countOfVisibleItemSlots_;
}

void LocationsTab::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    QRect bkgd(0, 0, geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());

    WidgetSwitcher *currentWidget = getCurrentWidget();
    int count = currentWidget ? currentWidget->locationsView()->count() : 0;

    QFont font = FontManager::instance().getFont(12, QFont::Normal);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.setOpacity(0.6);

    QString text;
    if (count == 0) {
        text = "";
    } else if (curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
        text = tr("All locations (%1)").arg(count);
    } else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS) {
        text = tr("Favourites");
    } else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS) {
        text = tr("Static IPs");
    } else if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS) {
        text = tr("Custom configs");
    } else if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) {
        text = tr("Search");
    }

    if (preferences_->appSkin() == APP_SKIN::APP_SKIN_VAN_GOGH) {
        painter.drawText(QRect(16*G_SCALE, 18*G_SCALE, geometry().width() - 32*G_SCALE, headerHeight()*G_SCALE), Qt::AlignLeft, text);
    } else {
        painter.drawText(QRect(16*G_SCALE, 38*G_SCALE, geometry().width() - 32*G_SCALE, headerHeight()*G_SCALE), Qt::AlignLeft, text);
    }

    // Draw refresh button text or spinner
    if (!refreshButtonText_.isEmpty() && curTab_ != LOCATION_TAB_CONFIGURED_LOCATIONS && connectState_ == CONNECT_STATE_DISCONNECTED) {
        painter.setOpacity(OPACITY_FULL);

        if (pingsActive_) {
            // Show rotating spinner instead of text when pings are active
            QSharedPointer<IndependentPixmap> spinnerPixmap = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
            int spinnerSize = 16*G_SCALE;
            int scaledWidth = geometry().width();
            int spinnerX = scaledWidth - spinnerSize - 24*G_SCALE;
            int spinnerY = refreshButtonRect_.top() + (refreshButtonRect_.height() - spinnerSize) / 2;

            // Apply rotation animation
            painter.save();
            painter.translate(spinnerX + spinnerSize/2, spinnerY + spinnerSize/2);
            painter.rotate(currentSpinnerRotation_);
            spinnerPixmap->draw(-spinnerSize/2, -spinnerSize/2, spinnerSize, spinnerSize, &painter);
            painter.restore();
        } else {
            // Show refresh text
            QColor hoverColor = Qt::white;

            if (refreshButtonAnimationProgress_ > 0.0) {
                int r = refreshButtonColor_.red() + (hoverColor.red() - refreshButtonColor_.red()) * refreshButtonAnimationProgress_;
                int g = refreshButtonColor_.green() + (hoverColor.green() - refreshButtonColor_.green()) * refreshButtonAnimationProgress_;
                int b = refreshButtonColor_.blue() + (hoverColor.blue() - refreshButtonColor_.blue()) * refreshButtonAnimationProgress_;
                painter.setPen(QColor(r, g, b));
            } else {
                painter.setPen(refreshButtonColor_);
            }

            painter.drawText(refreshButtonRect_, Qt::AlignRight | Qt::AlignVCenter, refreshButtonText_);
        }
    }
}

void LocationsTab::onClickAllLocations()
{
    curTab_ = LOCATION_TAB_ALL_LOCATIONS;
    ///widgetAllLocations_->startAnimationWithPixmap(this->grab(QRect(0, TAB_HEADER_HEIGHT* G_SCALE, width(), height() - TAB_HEADER_HEIGHT* G_SCALE)));
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetSearchLocations_->hide();
    widgetAllLocations_->show();
    widgetAllLocations_->raise();
    updateRibbonVisibility();
    update();
}

void LocationsTab::onClickConfiguredLocations()
{
    curTab_ = LOCATION_TAB_CONFIGURED_LOCATIONS;
    ///widgetConfiguredLocations_->startAnimationWithPixmap(this->grab(QRect(0, TAB_HEADER_HEIGHT* G_SCALE, width(), height() - TAB_HEADER_HEIGHT* G_SCALE)));
    widgetAllLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetSearchLocations_->hide();
    widgetConfiguredLocations_->show();
    widgetConfiguredLocations_->raise();
    updateRibbonVisibility();
}

void LocationsTab::onClickStaticIpsLocations()
{
    curTab_ = LOCATION_TAB_STATIC_IPS_LOCATIONS;
    ///widgetStaticIpsLocations_->startAnimationWithPixmap(this->grab(QRect(0, TAB_HEADER_HEIGHT* G_SCALE, width(), height() - TAB_HEADER_HEIGHT* G_SCALE)));
    widgetAllLocations_->hide();
    widgetConfiguredLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetSearchLocations_->hide();
    widgetStaticIpsLocations_->show();
    widgetStaticIpsLocations_->raise();
    updateRibbonVisibility();
}

void LocationsTab::onClickFavoriteLocations()
{
    curTab_ = LOCATION_TAB_FAVORITE_LOCATIONS;
    ///widgetFavoriteLocations_->startAnimationWithPixmap(this->grab(QRect(0, TAB_HEADER_HEIGHT* G_SCALE, width(), height() - TAB_HEADER_HEIGHT* G_SCALE)));
    widgetAllLocations_->hide();
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetSearchLocations_->hide();
    widgetFavoriteLocations_->show();
    widgetFavoriteLocations_->raise();
    updateRibbonVisibility();
}

void LocationsTab::onClickSearchLocations()
{
    curTab_ = LOCATION_TAB_SEARCH_LOCATIONS;
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetAllLocations_->hide();
    widgetSearchLocations_->locationsView()->collapseAll();
    widgetSearchLocations_->locationsView()->scrollToTop();
    widgetSearchLocations_->show();
    widgetSearchLocations_->raise();
    updateRibbonVisibility();
}

void LocationsTab::onDeviceNameChanged(const QString &deviceName)
{
    staticIPDeviceInfo_->setDeviceName(deviceName);
}

void LocationsTab::onAddCustomConfigClicked()
{
    emit addCustomConfigClicked();
}

void LocationsTab::onLocationSelected(const LocationID &lid)
{
    emit selected(lid);
}

void LocationsTab::onClickedOnPremiumStarCity()
{
    emit clickedOnPremiumStarCity();
}

WidgetSwitcher *LocationsTab::getCurrentWidget() const
{
    switch (curTab_) {
        case LOCATION_TAB_ALL_LOCATIONS:
            return widgetAllLocations_;
        case LOCATION_TAB_FAVORITE_LOCATIONS:
            return widgetFavoriteLocations_;
        case LOCATION_TAB_STATIC_IPS_LOCATIONS:
            return widgetStaticIpsLocations_;
        case LOCATION_TAB_CONFIGURED_LOCATIONS:
            return widgetConfiguredLocations_;
        case LOCATION_TAB_SEARCH_LOCATIONS:
            return widgetSearchLocations_;
        default:
            WS_ASSERT(false);
            return nullptr;
    }
}

void LocationsTab::updateLocationWidgetsGeometry(int newHeight)
{
    currentLocationListHeight_ = newHeight;

    int topAreaHeight = headerHeight()*G_SCALE;
    int scaledHeight = qCeil(newHeight * G_SCALE);
    int scaledWidth = WINDOW_WIDTH * G_SCALE;
    int ribbonHeight = 0;
    if (isRibbonVisible_) {
        if (curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
            ribbonHeight = 72*G_SCALE;
        } else {
            ribbonHeight = RIBBON_HEIGHT*G_SCALE;
        }
    }
    int ribbonOffset = qCeil(COVER_LAST_ITEM_LINE*G_SCALE);
    int alphaOffset = preferences_->appSkin() == APP_SKIN::APP_SKIN_ALPHA ? 16*G_SCALE : 0;

    widgetAllLocations_->setGeometry(0, topAreaHeight, scaledWidth, scaledHeight - ribbonHeight);
    widgetFavoriteLocations_->setGeometry(0, topAreaHeight, scaledWidth, scaledHeight);
    widgetStaticIpsLocations_->setGeometry(0, topAreaHeight, scaledWidth, scaledHeight - ribbonHeight);
    widgetConfiguredLocations_->setGeometry(0, topAreaHeight, scaledWidth, scaledHeight - ribbonHeight);
    widgetSearchLocations_->setGeometry(0, topAreaHeight, scaledWidth, scaledHeight);

    QFont font = FontManager::instance().getFont(12, QFont::Normal);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(refreshButtonText_);
    int textHeight = fm.height();

    if (preferences_->appSkin() == APP_SKIN::APP_SKIN_VAN_GOGH) {
        refreshButtonRect_ = QRect(scaledWidth - textWidth - 24*G_SCALE, 18*G_SCALE, textWidth, textHeight);
    } else {
        refreshButtonRect_ = QRect(scaledWidth - textWidth - 24*G_SCALE, 38*G_SCALE, textWidth, textHeight);
    }

    // ribbon geometry
    staticIPDeviceInfo_->setGeometry(0, scaledHeight - ribbonOffset + alphaOffset, scaledWidth, ribbonHeight);
    configFooterInfo_->setGeometry(0, scaledHeight - ribbonOffset + alphaOffset, scaledWidth, ribbonHeight);
    upgradeBanner_->setGeometry(8*G_SCALE, scaledHeight - 26*G_SCALE + alphaOffset, scaledWidth - 16*G_SCALE, 64*G_SCALE);

    updateScaling();
}

void LocationsTab::updateScaling()
{
    widgetAllLocations_->updateScaling();
    widgetFavoriteLocations_->updateScaling();
    widgetStaticIpsLocations_->updateScaling();
    widgetConfiguredLocations_->updateScaling();
    widgetSearchLocations_->updateScaling();
    configFooterInfo_->updateScaling();
    update();
}

int LocationsTab::unscaledHeightOfItemViewport()
{
    return LOCATION_ITEM_HEIGHT * countOfVisibleItemSlots_;
}

void LocationsTab::setShowLocationLoad(bool showLocationLoad)
{
    widgetAllLocations_->locationsView()->setShowLocationLoad(showLocationLoad);
    widgetConfiguredLocations_->locationsView()->setShowLocationLoad(showLocationLoad);
    widgetStaticIpsLocations_->locationsView()->setShowLocationLoad(showLocationLoad);
    widgetFavoriteLocations_->locationsView()->setShowLocationLoad(showLocationLoad);
    widgetSearchLocations_->locationsView()->setShowLocationLoad(showLocationLoad);
}

void LocationsTab::setCustomConfigsPath(QString path)
{
    configFooterInfo_->setText(path);
    updateCustomConfigsEmptyListVisibility();
    updateRibbonVisibility();
}

void LocationsTab::updateCustomConfigsEmptyListVisibility()
{
    WS_ASSERT(configFooterInfo_ != nullptr);
    if (configFooterInfo_->text().isEmpty()) {
        widgetConfiguredLocations_->emptyListWidget()->setText(
            tr("Choose the directory that contains custom configs you wish to display here"), 160);
        widgetConfiguredLocations_->emptyListWidget()->setButton(tr("Choose"));
    } else {
        widgetConfiguredLocations_->emptyListWidget()->setText(
            tr("The selected directory contains no custom configs"), 160);
        widgetConfiguredLocations_->emptyListWidget()->setButton(QString());
    }
}

void LocationsTab::updateRibbonVisibility()
{
    isRibbonVisible_ = false;

    if (curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
        configFooterInfo_->hide();
        staticIPDeviceInfo_->hide();

        if (!isUnlimitedData_) {
            upgradeBanner_->show();
            upgradeBanner_->raise();
            isRibbonVisible_ = true;
        } else {
            upgradeBanner_->hide();
        }
    } else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS) {
        configFooterInfo_->hide();
        upgradeBanner_->hide();

        if (widgetStaticIpsLocations_->locationsView()->isEmptyList()) {
            staticIPDeviceInfo_->hide();
        } else {
            staticIPDeviceInfo_->show();
            staticIPDeviceInfo_->raise();
            isRibbonVisible_ = true;
        }
    } else if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS) {
        staticIPDeviceInfo_->hide();
        upgradeBanner_->hide();

        const bool is_custom_configs_dir_set = !configFooterInfo_->text().isEmpty();
        if (!is_custom_configs_dir_set) {
            configFooterInfo_->hide();
        } else {
            configFooterInfo_->show();
            configFooterInfo_->raise();
            isRibbonVisible_ = true;
        }
    } else {
        configFooterInfo_->hide();
        staticIPDeviceInfo_->hide();
        upgradeBanner_->hide();
    }

    if (currentLocationListHeight_ != 0) {
        updateLocationWidgetsGeometry(currentLocationListHeight_);
    }
}

void LocationsTab::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    updateRibbonVisibility();
    updateLocationWidgetsGeometry(currentLocationListHeight_);
}

void LocationsTab::onLanguageChanged()
{
    widgetSearchLocations_->emptyListWidget()->setText(tr("No locations found"), 120);
    widgetFavoriteLocations_->emptyListWidget()->setText(tr("Nothing to see here..."), 120);
    widgetStaticIpsLocations_->emptyListWidget()->setText(tr("You don't have any Static IPs"), 120);
    widgetAllLocations_->emptyListWidget()->setText(tr("No locations"), 120);
    // Delete old button first
    widgetStaticIpsLocations_->emptyListWidget()->setButton("");
    widgetConfiguredLocations_->emptyListWidget()->setButton("");
    // Set new buttons
    widgetStaticIpsLocations_->emptyListWidget()->setButton(tr("Buy"));

    refreshButtonText_ = tr("Refresh Pings");

    updateCustomConfigsEmptyListVisibility();
}

int LocationsTab::headerHeight() const
{
    if (preferences_->appSkin() == APP_SKIN::APP_SKIN_VAN_GOGH) {
        return 40;
    } else {
        return 60;
    }
}

void LocationsTab::onSearchFilterChanged(const QString &filter)
{
    locationsModelManager_->setFilterString(filter);
    if (filter.isEmpty()) {
        widgetSearchLocations_->locationsView()->collapseAll();
    } else {
        widgetSearchLocations_->locationsView()->expandAll();
    }
    widgetSearchLocations_->locationsView()->scrollToTop();
    update();
}

void LocationsTab::onLocationsKeyPressed(QKeyEvent *event)
{
    WidgetSwitcher *curWidget = getCurrentWidget();
    if (curWidget) {
        curWidget->locationsView()->handleKeyPressEvent(event);
    }
}

void LocationsTab::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    isUnlimitedData_ = (bytesMax == -1);
    updateRibbonVisibility();
    upgradeBanner_->setDataRemaining(bytesUsed, bytesMax);
}

void LocationsTab::onConnectStateChanged(const types::ConnectState &connectState)
{
    connectState_ = connectState.connectState;
    update();
}

void LocationsTab::onPingsStarted()
{
    pingsActive_ = true;
    spinnerRotationAnimation_->start();
    update();
}

void LocationsTab::onPingsFinished()
{
    pingsActive_ = false;
    spinnerRotationAnimation_->stop();
    currentSpinnerRotation_ = 0.0;
    update();
}

void LocationsTab::mousePressEvent(QMouseEvent *event)
{
    if (refreshButtonRect_.contains(event->pos()) && isRefreshButtonVisible()) {
        setCursor(Qt::ArrowCursor);
        emit refreshClicked();
    }
    QWidget::mousePressEvent(event);
}

void LocationsTab::mouseMoveEvent(QMouseEvent *event)
{
    bool isHovered = refreshButtonRect_.contains(event->pos()) && isRefreshButtonVisible();

    if (isHovered) {
        refreshButtonColorAnimation_->setStartValue(refreshButtonAnimationProgress_);
        refreshButtonColorAnimation_->setEndValue(1.0);
        refreshButtonColorAnimation_->start();
        setCursor(Qt::PointingHandCursor);
    } else {
        refreshButtonColorAnimation_->setStartValue(refreshButtonAnimationProgress_);
        refreshButtonColorAnimation_->setEndValue(0.0);
        refreshButtonColorAnimation_->start();
        setCursor(Qt::ArrowCursor);
    }

    QWidget::mouseMoveEvent(event);
}

void LocationsTab::leaveEvent(QEvent *event)
{
    refreshButtonColorAnimation_->setStartValue(refreshButtonAnimationProgress_);
    refreshButtonColorAnimation_->setEndValue(0.0);
    refreshButtonColorAnimation_->start();
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

bool LocationsTab::isRefreshButtonVisible() const
{
    return connectState_ == CONNECT_STATE_DISCONNECTED && curTab_ != LOCATION_TAB_CONFIGURED_LOCATIONS && !pingsActive_;
}

} // namespace GuiLocations
