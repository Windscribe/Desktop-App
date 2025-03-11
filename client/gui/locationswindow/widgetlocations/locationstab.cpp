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
  , curTab_(LOCATION_TAB_ALL_LOCATIONS)
  , lastTab_(LOCATION_TAB_ALL_LOCATIONS)
  , tabPress_(LOCATION_TAB_NONE)
  , filterText_("")
  , searchTabSelected_(false)
  , curTabMouseOver_(LOCATION_TAB_NONE)
  , countOfVisibleItemSlots_(7)
  , currentLocationListHeight_(0)
  , isRibbonVisible_(false)
  , showAllTabs_(true)
  , tabBackgroundColor_(14, 25, 38)
{
    setMouseTracking(true);
    curCursorShape_ = Qt::ArrowCursor;

    connect(preferences_, &Preferences::appSkinChanged, this, &LocationsTab::onAppSkinChanged);

    searchButtonPosUnscaled_ = LAST_TAB_ICON_POS_X;
    searchButton_ = new CommonWidgets::IconButtonWidget("locations/SEARCH_ICON", this);
    searchButton_->setUnhoverHoverOpacity(TAB_OPACITY_DIM, TAB_OPACITY_DIM);
    connect(searchButton_, &CommonWidgets::IconButtonWidget::clicked, this, &LocationsTab::onSearchButtonClicked);
    connect(searchButton_, &CommonWidgets::IconButtonWidget::hoverEnter, this, &LocationsTab::onSearchButtonHoverEnter);
    connect(searchButton_, &CommonWidgets::IconButtonWidget::hoverLeave, this, &LocationsTab::onSearchButtonHoverLeave);
    searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

    searchButtonPosAnimation_.setDuration(SEARCH_BUTTON_POS_ANIMATION_DURATION);
    searchButtonPosAnimation_.setStartValue(preferences_->appSkin() == APP_SKIN_VAN_GOGH ? FIRST_TAB_ICON_POS_X_VAN_GOGH : FIRST_TAB_ICON_POS_X); // keep animation unaware of scaling
    searchButtonPosAnimation_.setEndValue(LAST_TAB_ICON_POS_X);
    connect(&searchButtonPosAnimation_, &QVariantAnimation::valueChanged, this, &LocationsTab::onSearchButtonPosAnimationValueChanged);

    searchCancelButton_ = new CommonWidgets::IconButtonWidget("locations/SEARCH_CANCEL_ICON", this);
    searchCancelButton_->setUnhoverHoverOpacity(TAB_OPACITY_DIM, OPACITY_FULL);
    connect(searchCancelButton_, &CommonWidgets::IconButtonWidget::clicked, this, &LocationsTab::onSearchCancelButtonClicked);
    searchCancelButton_->move(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
    searchCancelButton_->hide();

    searchLineEdit_ = new CommonWidgets::CustomMenuLineEdit(this);
    searchLineEdit_->setFont(FontManager::instance().getFont(14, false));
    searchLineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    searchLineEdit_->setFrame(false);
    searchLineEdit_->installEventFilter(this);
    connect(searchLineEdit_, &CommonWidgets::CustomMenuLineEdit::textChanged, this, &LocationsTab::onSearchLineEditTextChanged);
    connect(searchLineEdit_, &CommonWidgets::CustomMenuLineEdit::focusOut, this, &LocationsTab::onSearchLineEditFocusOut);

    searchLineEdit_->hide();

    updateIconRectsAndLine();

    curWhiteLinePos_ = (rcAllLocationsIcon_.center().x() + 1) * G_SCALE;
    connect(&whiteLineAnimation_, &QVariantAnimation::valueChanged, this, &LocationsTab::onWhiteLinePosChanged);

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

    updateLocationWidgetsGeometry(unscaledHeightOfItemViewport());

    searchTypingDelayTimer_.setSingleShot(true);
    searchTypingDelayTimer_.setInterval(100);
    connect(&searchTypingDelayTimer_, &QTimer::timeout, this, &LocationsTab::onSearchTypingDelayTimerTimeout);

    delayShowIconsTimer_.setSingleShot(true);
    delayShowIconsTimer_.setInterval(SEARCH_BUTTON_POS_ANIMATION_DURATION+100);
    connect(&delayShowIconsTimer_, &QTimer::timeout, this, &LocationsTab::onDelayShowIconsTimerTimeout);

    connect(locationsModelManager, &gui_locations::LocationsModelManager::deviceNameChanged, this, &LocationsTab::onDeviceNameChanged);
    updateCustomConfigsEmptyListVisibility();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LocationsTab::onLanguageChanged);
    onLanguageChanged();

    // We default to showing the all locations tab, so only changeTab if the last session exited on a different tab.
    if (curTab_ != PersistentState::instance().lastLocationTab()) {
        changeTab(PersistentState::instance().lastLocationTab(), false);
    }
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

void LocationsTab::setOnlyConfigTabVisible(bool onlyConfig)
{
    if (onlyConfig)
    {
        showAllTabs_ = false;
        changeTab(LOCATION_TAB_CONFIGURED_LOCATIONS);
    }
    else
    {
        showAllTabs_ = true;
        changeTab(LOCATION_TAB_ALL_LOCATIONS);
    }
}

void LocationsTab::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (isWhiteAnimationActive())
    {
        update();
    }

    // cover no background between ribbon and bottom handle (static and config)
    QPainter painter(this);
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());
    drawTabRegion(painter, QRect(0, 0, width(), TAB_HEADER_HEIGHT * G_SCALE));
}

void LocationsTab::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pt = QCursor::pos();
    pt = mapFromGlobal(pt);

    const int addMargin = 5*G_SCALE;

    if (showAllTabs_)
    {
        if (!searchTabSelected_)
        {
            if (rcAllLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_ALL_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcAllLocationsIcon_, tr("All"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcConfiguredLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_CONFIGURED_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcConfiguredLocationsIcon_, tr("Configured"), 9 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcStaticIpsLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_STATIC_IPS_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcStaticIpsLocationsIcon_, tr("Static IPs"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcFavoriteLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_FAVORITE_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcFavoriteLocationsIcon_, tr("Favourites"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else
            {
                curTabMouseOver_ = LOCATION_TAB_NONE;
                setArrowCursor();

                TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
            }
        }
        else
        {
            curTabMouseOver_ = LOCATION_TAB_NONE;
            setArrowCursor();

            TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void LocationsTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (curTabMouseOver_ != LOCATION_TAB_NONE)
        {
            tabPress_ = curTabMouseOver_;
        }
    }
}

void LocationsTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (curTabMouseOver_ != LOCATION_TAB_NONE &&  curTabMouseOver_ != curTab_ && tabPress_ == curTabMouseOver_)
        {
            // Currently this should only handle non-search icons
            if (!searchTabSelected_) {
                changeTab(curTabMouseOver_);
            }
        }
    }
}


void LocationsTab::changeTab(LOCATION_TAB newTab, bool animateChange)
{
    lastTab_ = curTab_;
    curTab_ = newTab;

    // No need to persist the search tab as the last tab.  If we're on the search tab
    // and locations is collapsed, either by a user action or the app exiting, we'll
    // transition back to lastTab_ and persist that tab.
    if (curTab_ != LOCATION_TAB_SEARCH_LOCATIONS) {
        PersistentState::instance().setLastLocationTab(curTab_);
    }

    updateTabIconRects();

    int endWhiteLinePos;
    if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS) {
        endWhiteLinePos = rcConfiguredLocationsIcon_.center().x();
        onClickConfiguredLocations();
    } else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS) {
        endWhiteLinePos = rcStaticIpsLocationsIcon_.center().x();
        onClickStaticIpsLocations();
    } else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS) {
        endWhiteLinePos = rcFavoriteLocationsIcon_.center().x();
        onClickFavoriteLocations();
    } else if (curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
        endWhiteLinePos = rcAllLocationsIcon_.center().x();
        onClickAllLocations();
    } else if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) {
        // no line movement for search locations tab, it is hidden
        endWhiteLinePos = curWhiteLinePos_;
        onClickSearchLocations();
    } else {
        endWhiteLinePos = 0;
        WS_ASSERT(false);
    }

    if (animateChange) {
        whiteLineAnimation_.stop();
        whiteLineAnimation_.setStartValue(curWhiteLinePos_);
        whiteLineAnimation_.setEndValue(endWhiteLinePos);
        whiteLineAnimation_.setDuration(ANIMATION_DURATION);
        whiteLineAnimation_.start();
    } else {
        curWhiteLinePos_ = endWhiteLinePos;
    }

    update();
}


void LocationsTab::leaveEvent(QEvent *event)
{
    curTabMouseOver_ = LOCATION_TAB_NONE;
    setArrowCursor();
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
    QWidget::leaveEvent(event);
}

bool LocationsTab::eventFilter(QObject *object, QEvent *event)
{
    if (object == searchLineEdit_)
    {
        if (event->type() == QEvent::KeyRelease)
        {
            // qDebug() << "Filtered KeyRelease from search line edit";
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape)
            {
                if (searchLineEdit_->text() == "")
                {
                    hideSearchTab();
                }
                else
                {
                    searchLineEdit_->setText("");
                }
            }
            return true;
        }
        else if (event->type() == QEvent::KeyPress)
        {
            // block KeyPresses too on up/down to prevent lineedit cursor movement
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up ||
                keyEvent->key() == Qt::Key_Down)
            {
                passEventToLocationWidget(keyEvent);
                return true;
            }
            else if (keyEvent->key() == Qt::Key_PageDown || keyEvent->key() == Qt::Key_PageUp
                     || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                passEventToLocationWidget(keyEvent);
            }
        }
    }
    return QWidget::eventFilter(object, event);
}

void LocationsTab::onClickAllLocations()
{
    ///widgetAllLocations_->startAnimationWithPixmap(this->grab(QRect(0, TAB_HEADER_HEIGHT* G_SCALE, width(), height() - TAB_HEADER_HEIGHT* G_SCALE)));
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetSearchLocations_->hide();
    widgetAllLocations_->show();
    widgetAllLocations_->raise();
    updateRibbonVisibility();
}

void LocationsTab::onClickConfiguredLocations()
{
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

void LocationsTab::switchToTabAndRestoreCursorToAccentedItem(LOCATION_TAB locationTab)
{
    if (locationTab == LOCATION_TAB_SEARCH_LOCATIONS) // going to search tab
    {
        showSearchTab(); // this will handle changeTab(..)
    }
    else if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) // leaving search tab
    {
        hideSearchTab(); // handles changeTab(..)
    }
    else // any other transition
    {
        changeTab(locationTab);
    }
    getCurrentWidget()->locationsView()->updateSelected();
}

void LocationsTab::onWhiteLinePosChanged(const QVariant &value)
{
    curWhiteLinePos_ = value.toInt();
    update();
}

void LocationsTab::onDeviceNameChanged(const QString &deviceName)
{
    staticIPDeviceInfo_->setDeviceName(deviceName);
}

void LocationsTab::onAddCustomConfigClicked()
{
    emit addCustomConfigClicked();
}

void LocationsTab::onSearchButtonHoverEnter()
{
    QPoint buttonGlobalPoint = mapToGlobal(searchButton_->geometry().topLeft());
    int posX = buttonGlobalPoint.x() + searchButton_->geometry().width()/2;
    int posY = buttonGlobalPoint.y() - 5 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_TAB_INFO);
    ti.x = posX;
    ti.y = posY;
    ti.title = tr("Search");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    TooltipController::instance().showTooltipBasic(ti);
}

void LocationsTab::onSearchButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
}

void LocationsTab::onSearchButtonClicked()
{
    showSearchTab();
}

void LocationsTab::onSearchCancelButtonClicked()
{
    hideSearchTab();
}

void LocationsTab::onSearchButtonPosAnimationValueChanged(const QVariant &value)
{
    searchButtonPosUnscaled_ = value.toInt();
    searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
}

void LocationsTab::onDelayShowIconsTimerTimeout()
{
    searchTabSelected_ = false;
    update();
}

void LocationsTab::onSearchTypingDelayTimerTimeout()
{
    locationsModelManager_->setFilterString(filterText_);
    if (filterText_.isEmpty())
        widgetSearchLocations_->locationsView()->collapseAll();
    else
        widgetSearchLocations_->locationsView()->expandAll();

    widgetSearchLocations_->locationsView()->scrollToTop();
}

void LocationsTab::onSearchLineEditTextChanged(QString text)
{
    filterText_ = text;
    searchTypingDelayTimer_.start();
}

void LocationsTab::onSearchLineEditFocusOut()
{
    focusOutTimer_.restart();
}

void LocationsTab::onLocationSelected(const LocationID &lid)
{
    emit selected(lid);
}

void LocationsTab::onClickedOnPremiumStarCity()
{
    emit clickedOnPremiumStarCity();
}

void LocationsTab::drawTabRegion(QPainter &painter, const QRect &rc)
{
    painter.fillRect(rc, QBrush(tabBackgroundColor_));

    if (!searchTabSelected_)
    {
        // draw white line
        drawBottomLine(painter, rc.bottom(), curWhiteLinePos_);

        // Draw Icons
        {
            QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/CONFIG_ICON");
            painter.setOpacity(curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
            p->draw(rcConfiguredLocationsIcon_.left(), rcConfiguredLocationsIcon_.top(), &painter);
        }
        if (showAllTabs_)
        {
            {
                QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/STATIC_IP_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcStaticIpsLocationsIcon_.left(), rcStaticIpsLocationsIcon_.top(), &painter);
            }
            {
                QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcFavoriteLocationsIcon_.left(), rcFavoriteLocationsIcon_.top(), &painter);
            }
            {
                QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/ALL_LOCATION_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_ALL_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcAllLocationsIcon_.left(), rcAllLocationsIcon_.top(), &painter);
            }
        }
    }
}

void LocationsTab::drawBottomLine(QPainter &painter, int bottom, int whiteLinePos)
{
    // draw white line
    painter.fillRect(QRect((whiteLinePos - WHITE_LINE_WIDTH / 2 *G_SCALE), bottom-G_SCALE*2 + 1, WHITE_LINE_WIDTH*G_SCALE, G_SCALE*2), QBrush(Qt::white));
}

void LocationsTab::setArrowCursor()
{
    if (curCursorShape_ != Qt::ArrowCursor)
    {
        setCursor(Qt::ArrowCursor);
        curCursorShape_ = Qt::ArrowCursor;
    }
}

void LocationsTab::setPointingHandCursor()
{
    if (curCursorShape_ != Qt::PointingHandCursor)
    {
        setCursor(Qt::PointingHandCursor);
        curCursorShape_ = Qt::PointingHandCursor;
    }
}

bool LocationsTab::isWhiteAnimationActive()
{
    return whiteLineAnimation_.state() == QAbstractAnimation::Running;
}

void LocationsTab::passEventToLocationWidget(QKeyEvent *event)
{
    WidgetSwitcher *curWidget = getCurrentWidget();
    if (curWidget)
        curWidget->locationsView()->handleKeyPressEvent(event);
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

void LocationsTab::updateTabIconRects()
{
    searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
    searchCancelButton_->move(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

    int pos[4] = { 0, 0, 0, 0 };

    int totalWidthForFirst4TabsWithSpacing;
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        pos[0] = FIRST_TAB_ICON_POS_X_VAN_GOGH;
        pos[1] = pos[0] + 48;
        pos[2] = pos[1] + 48;
        pos[3] = pos[2] + 48;
    }
    else
    {
        totalWidthForFirst4TabsWithSpacing = LAST_TAB_ICON_POS_X - FIRST_TAB_ICON_POS_X;
        int iconWidths = 16 + 16 + 16 + 18;
        int totalSpacing = totalWidthForFirst4TabsWithSpacing - iconWidths;
        int eachSpacing = totalSpacing/4;

        pos[0] = FIRST_TAB_ICON_POS_X;
        pos[1] = pos[0] + eachSpacing + 16;
        pos[2] = pos[1] + eachSpacing + 16;
        pos[3] = pos[2] + eachSpacing + 16;
    }

    rcAllLocationsIcon_.       setRect(pos[0]*G_SCALE, TOP_TAB_MARGIN*G_SCALE, 16*G_SCALE, 16*G_SCALE);
    rcFavoriteLocationsIcon_.  setRect(pos[1]*G_SCALE, TOP_TAB_MARGIN*G_SCALE, 16*G_SCALE, 14*G_SCALE);
    rcStaticIpsLocationsIcon_. setRect(pos[2]*G_SCALE, TOP_TAB_MARGIN*G_SCALE, 16*G_SCALE, 16*G_SCALE);
    if (showAllTabs_)
    {
        rcConfiguredLocationsIcon_.setRect(pos[3]*G_SCALE, TOP_TAB_MARGIN*G_SCALE, 18*G_SCALE, 16*G_SCALE);
        searchButton_->show();
    }
    else
    {
        rcConfiguredLocationsIcon_.setRect(LAST_TAB_ICON_POS_X*G_SCALE, TOP_TAB_MARGIN*G_SCALE, 18*G_SCALE, 16*G_SCALE);
        searchButton_->hide();
    }

}

void LocationsTab::updateIconRectsAndLine()
{
    updateTabIconRects();

    // update white line pos
    if (curTab_ == LOCATION_TAB_ALL_LOCATIONS)                  curWhiteLinePos_ = (rcAllLocationsIcon_.center().x() + 1*G_SCALE);
    else if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS)      curWhiteLinePos_ = (rcConfiguredLocationsIcon_.center().x() + 1*G_SCALE);
    else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS)      curWhiteLinePos_ = (rcStaticIpsLocationsIcon_.center().x() + 1*G_SCALE);
    else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS)        curWhiteLinePos_ = (rcFavoriteLocationsIcon_.center().x() + 1*G_SCALE);
    // not drawing line for widgetSearchLocations_

    update();
}

void LocationsTab::updateLocationWidgetsGeometry(int newHeight)
{
    const int kRibbonHeight = isRibbonVisible_ ? RIBBON_HEIGHT : 0;

    currentLocationListHeight_ = newHeight;

    int scaledHeight = qCeil(newHeight * G_SCALE);
    int scaledWidth = WINDOW_WIDTH * G_SCALE;
    int scaledTabHeaderHeight = qCeil(TAB_HEADER_HEIGHT * G_SCALE);
    int ribbonHeight = (kRibbonHeight * G_SCALE);
    int ribbonOffset = qCeil(COVER_LAST_ITEM_LINE*G_SCALE);

    widgetAllLocations_->setGeometry(        0, scaledTabHeaderHeight, scaledWidth, scaledHeight);
    widgetFavoriteLocations_->setGeometry(   0, scaledTabHeaderHeight, scaledWidth, scaledHeight);
    widgetStaticIpsLocations_->setGeometry(  0, scaledTabHeaderHeight, scaledWidth, scaledHeight - ribbonHeight);
    widgetConfiguredLocations_->setGeometry( 0, scaledTabHeaderHeight, scaledWidth, scaledHeight - ribbonHeight);
    widgetSearchLocations_->setGeometry( 0, scaledTabHeaderHeight, scaledWidth, scaledHeight);

    // ribbon geometry
    staticIPDeviceInfo_->setGeometry( 0, scaledHeight - ribbonOffset, scaledWidth, ribbonHeight);
    configFooterInfo_->setGeometry(   0, scaledHeight - ribbonOffset, scaledWidth, ribbonHeight);

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

    searchButton_->setGeometry(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16*G_SCALE, 16*G_SCALE);
    searchCancelButton_->setGeometry(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16*G_SCALE, 16*G_SCALE);
    searchLineEdit_->setFont(FontManager::instance().getFont(14, false));
    searchLineEdit_->setGeometry(((preferences_->appSkin() == APP_SKIN_VAN_GOGH ? FIRST_TAB_ICON_POS_X_VAN_GOGH : FIRST_TAB_ICON_POS_X) + 32)*G_SCALE,
                                 2*G_SCALE,
                                 (LAST_TAB_ICON_POS_X - (preferences_->appSkin() == APP_SKIN_VAN_GOGH ? FIRST_TAB_ICON_POS_X_VAN_GOGH : FIRST_TAB_ICON_POS_X) - 32)*G_SCALE,
                                 40*G_SCALE);
}

void LocationsTab::updateLanguage()
{
}

void LocationsTab::showSearchTab()
{
    if (showAllTabs_)
    {
        searchButton_->setEnabled(false);

        QTimer::singleShot(100, [this]()
        {
            if (!searchTabSelected_) {
                changeTab(LOCATION_TAB_SEARCH_LOCATIONS);
                update();

                searchButtonPosAnimation_.stop();
                searchButtonPosAnimation_.setStartValue(preferences_->appSkin() == APP_SKIN_VAN_GOGH ? FIRST_TAB_ICON_POS_X_VAN_GOGH : FIRST_TAB_ICON_POS_X);
                searchButtonPosAnimation_.setDirection(QAbstractAnimation::Backward);
                searchButtonPosAnimation_.start();

                // delay so cursor and cancel don't appear before search slides over
                QTimer::singleShot(SEARCH_BUTTON_POS_ANIMATION_DURATION+100, [this](){
                    // check no interruption by hideSearchTab() -- could happen on fast searchClick->close locations tray
                    if (!delayShowIconsTimer_.isActive() && searchTabSelected_)
                    {
                        searchCancelButton_->show();
                        searchLineEdit_->show();
                        searchLineEdit_->setFocus();
                    }
                });
                searchTabSelected_ = true;
            }
        });
    }
}

void LocationsTab::hideSearchTab()
{
    if (searchTabSelected_)
    {
        searchButton_->setEnabled(true);
        searchButtonPosAnimation_.stop();
        searchButtonPosAnimation_.setDirection(QAbstractAnimation::Forward);
        searchButtonPosAnimation_.start();

        searchCancelButton_->hide();
        searchLineEdit_->hide();
        searchLineEdit_->clear();

        changeTab(lastTab_);

        // let animation finish before showing all tabs
        delayShowIconsTimer_.start();
    }
}

void LocationsTab::hideSearchTabWithoutAnimation()
{
    if (searchTabSelected_)
    {
        searchButtonPosAnimation_.stop();
        searchButton_->setEnabled(true);
        searchButtonPosUnscaled_ = LAST_TAB_ICON_POS_X;
        searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

        searchCancelButton_->hide();
        searchLineEdit_->hide();
        searchLineEdit_->clear();

        changeTab(lastTab_, false);

        searchTabSelected_ = false;
    }
}

bool LocationsTab::handleKeyPressEvent(QKeyEvent *event)
{
    TooltipController::instance().hideAllTooltips();

    if (event->key() == Qt::Key_Right)
    {
        if (showAllTabs_)
        {
            int curTabInt = static_cast<int>(curTab_);
            if (curTabInt < LOCATION_TAB_LAST)
            {
                curTabInt++;
                switchToTabAndRestoreCursorToAccentedItem(static_cast<LOCATION_TAB>(curTabInt));
            }
        }
        return true;
    }
    else if (event->key() == Qt::Key_Left)
    {
        if (showAllTabs_)
        {
            int curTabInt = static_cast<int>(curTab_);
            if (curTabInt > LOCATION_TAB_FIRST)
            {
                curTabInt--;
                switchToTabAndRestoreCursorToAccentedItem(static_cast<LOCATION_TAB>(curTabInt));
            }
        }
        return true;
    }
    else if (event->key() == Qt::Key_Tab)
    {
        // bring focus back into search line edit when focus is elsewhere
        // Strage behaviour on Mac (at least) -- issue probably exists in event system
        // can't see to use searchLineEdit_->hasFocus() -- always false
        if (focusOutTimer_.elapsed() > 200)
        {
            searchLineEdit_->setFocus();
        }
        return true;
    }
    else if (event->key() == Qt::Key_Up ||
             event->key() == Qt::Key_Down ||
             event->key() == Qt::Key_PageDown ||
             event->key() == Qt::Key_PageUp ||
             event->key() == Qt::Key_Return ||
             event->key() == Qt::Key_Enter )
    {
        passEventToLocationWidget(event);
        return true;
    }

    // set locations tab to search screen
    if (curTab_ != LOCATION_TAB_SEARCH_LOCATIONS)
    {
        // prevent space from triggering search when intention is to close locations tray
        if (!event->text().isEmpty() && event->key() != Qt::Key_Space) {
            switchToTabAndRestoreCursorToAccentedItem(LOCATION_TAB_SEARCH_LOCATIONS);

            // Adding text to search text must be handled in the keyPress instead of keyRelease because
            // during focus transition from locationsTab -> searchLineEdit: mainWindow no longer fires keyReleaseEvents
            // while searchLineEdit has not yet picked up the focus (it uses keyPressEvents too)
            // qDebug() << "Appending text: " << event->text();
            searchLineEdit_->appendText(event->text());
            searchLineEdit_->setFocus();
            return true;
        }
    }
    else
    {
        searchLineEdit_->appendText(event->text());
        searchLineEdit_->setFocus();
        return true;
    }
    return false;
}

LOCATION_TAB LocationsTab::currentTab()
{
    return curTab_;
}

void LocationsTab::setTab(LOCATION_TAB tab)
{
    switchToTabAndRestoreCursorToAccentedItem(tab);
}

int LocationsTab::unscaledHeightOfItemViewport()
{
    return LOCATION_ITEM_HEIGHT * countOfVisibleItemSlots_;
}

void LocationsTab::setLatencyDisplay(LATENCY_DISPLAY_TYPE l)
{
    bool isShowLatencyInMs = (l == LATENCY_DISPLAY_MS);
    widgetAllLocations_->locationsView()->setShowLatencyInMs(isShowLatencyInMs);
    widgetConfiguredLocations_->locationsView()->setShowLatencyInMs(isShowLatencyInMs);
    widgetStaticIpsLocations_->locationsView()->setShowLatencyInMs(isShowLatencyInMs);
    widgetFavoriteLocations_->locationsView()->setShowLatencyInMs(isShowLatencyInMs);
    widgetSearchLocations_->locationsView()->setShowLatencyInMs(isShowLatencyInMs);
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

void LocationsTab::rectHoverEnter(QRect buttonRect, QString text, int offsetX, int offsetY)
{
    QPoint buttonGlobalPoint = mapToGlobal(buttonRect.topLeft());
    int posX = buttonGlobalPoint.x() + offsetX;
    int posY = buttonGlobalPoint.y() + offsetY;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_TAB_INFO);
    ti.x = posX;
    ti.y = posY;
    ti.title = text;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    TooltipController::instance().showTooltipBasic(ti);
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

    if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS)
    {
        configFooterInfo_->hide();

        if (widgetStaticIpsLocations_->locationsView()->isEmptyList()) {
            staticIPDeviceInfo_->hide();
        } else {
            staticIPDeviceInfo_->show();
            staticIPDeviceInfo_->raise();
            isRibbonVisible_ = true;
        }
    }
    else if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS)
    {
        staticIPDeviceInfo_->hide();
        const bool is_custom_configs_dir_set = !configFooterInfo_->text().isEmpty();
        if (!is_custom_configs_dir_set) {
            configFooterInfo_->hide();
        } else {
            configFooterInfo_->show();
            configFooterInfo_->raise();
            isRibbonVisible_ = true;
        }
    }
    else
    {
        configFooterInfo_->hide();
        staticIPDeviceInfo_->hide();
    }

    if (currentLocationListHeight_ != 0)
        updateLocationWidgetsGeometry(currentLocationListHeight_);
}

void LocationsTab::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    updateScaling();
    updateIconRectsAndLine();
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
    updateCustomConfigsEmptyListVisibility();
}

} // namespace GuiLocations
