#include "locationstab.h"

#include <QMessageBox>
#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltiptypes.h"
#include "utils/writeaccessrightschecker.h"
#include "tooltips/tooltipcontroller.h"
#include "widgetlocationssizes.h"

#include <QDebug>

extern QWidget *g_mainWindow;

namespace GuiLocations {


LocationsTab::LocationsTab(QWidget *parent, LocationsModel *locationsModel) : QWidget(parent)
  , curTab_(LOCATION_TAB_ALL_LOCATIONS)
  , tabPress_(LOCATION_TAB_NONE)
  , filterText_("")
  , searchTabSelected_(false)
  , curTabMouseOver_(LOCATION_TAB_NONE)
  , checkCustomConfigPathAccessRights_(false)
  , countOfVisibleItemSlots_(7)
  , currentLocationListHeight_(0)
  , isRibbonVisible_(false)
  , showAllTabs_(true)
  , tabBackgroundColor_(14, 25, 38)
{
    setMouseTracking(true);
    curCursorShape_ = Qt::ArrowCursor;

    searchButtonPosUnscaled_ = LAST_TAB_ICON_POS_X;
    searchButton_ = new CommonWidgets::IconButtonWidget("locations/SEARCH_ICON", this);
    searchButton_->setUnhoverHoverOpacity(TAB_OPACITY_DIM, TAB_OPACITY_DIM);
    connect(searchButton_, SIGNAL(clicked()), SLOT(onSearchButtonClicked()));
    connect(searchButton_, SIGNAL(hoverEnter()), SLOT(onSearchButtonHoverEnter()));
    connect(searchButton_, SIGNAL(hoverLeave()), SLOT(onSearchButtonHoverLeave()));
    searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

    searchButtonPosAnimation_.setDuration(SEARCH_BUTTON_POS_ANIMATION_DURATION);
    searchButtonPosAnimation_.setStartValue(FIRST_TAB_ICON_POS_X); // keep animation unaware of scaling
    searchButtonPosAnimation_.setEndValue(LAST_TAB_ICON_POS_X);
    connect(&searchButtonPosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSearchButtonPosAnimationValueChanged(QVariant)));

    searchCancelButton_ = new CommonWidgets::IconButtonWidget("locations/SEARCH_CANCEL_ICON", this);
    searchCancelButton_->setUnhoverHoverOpacity(TAB_OPACITY_DIM, OPACITY_FULL);
    connect(searchCancelButton_, SIGNAL(clicked()), SLOT(onSearchCancelButtonClicked()));
    searchCancelButton_->move(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
    searchCancelButton_->hide();

    searchLineEdit_ = new CommonWidgets::CustomMenuLineEdit(this);
    searchLineEdit_->setFont(*FontManager::instance().getFont(14, false));
    searchLineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    searchLineEdit_->setFrame(false);
    searchLineEdit_->installEventFilter(this);
    connect(searchLineEdit_, SIGNAL(textChanged(QString)), SLOT(onSearchLineEditTextChanged(QString)));
    connect(searchLineEdit_, SIGNAL(focusOut()), SLOT(onSearchLineEditFocusOut()));

    searchLineEdit_->hide();

    updateIconRectsAndLine();

    curWhiteLinePos_ = (rcAllLocationsIcon_.center().x() + 1) * G_SCALE;
    connect(&whiteLineAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onWhiteLinePosChanged(QVariant)));

    widgetAllLocations_ = new GuiLocations::WidgetLocations(this);

    widgetConfiguredLocations_ = new GuiLocations::WidgetCities(this, 6);
    widgetConfiguredLocations_->setEmptyListDisplayIcon("locations/FOLDER_ICON_BIG");
    connect(widgetConfiguredLocations_, SIGNAL(emptyListButtonClicked()),
                                        SLOT(onAddCustomConfigClicked()));
    widgetConfiguredLocations_->hide();

    widgetStaticIpsLocations_ = new GuiLocations::WidgetCities(this, 6);
    widgetStaticIpsLocations_->setEmptyListDisplayIcon("locations/STATIC_IP_ICON_BIG");
    widgetStaticIpsLocations_->setEmptyListDisplayText(tr("You don't have any Static IPs"), 120);
    widgetStaticIpsLocations_->setEmptyListButtonText(tr("Buy"));
    connect(widgetStaticIpsLocations_, SIGNAL(emptyListButtonClicked()),
                                       SIGNAL(addStaticIpClicked()));
    widgetStaticIpsLocations_->hide();

    widgetFavoriteLocations_ = new GuiLocations::WidgetCities(this);
    widgetFavoriteLocations_->setEmptyListDisplayIcon("locations/BROKEN_HEART_ICON");
    widgetFavoriteLocations_->setEmptyListDisplayText(tr("Nothing to see here..."), 120);
    widgetFavoriteLocations_->hide();

    staticIPDeviceInfo_ = new StaticIPDeviceInfo(this);
    connect(staticIPDeviceInfo_, SIGNAL(addStaticIpClicked()), SIGNAL(addStaticIpClicked()));
    staticIPDeviceInfo_->hide();

    configFooterInfo_ = new ConfigFooterInfo(this);
    configFooterInfo_->hide();
    connect(configFooterInfo_, SIGNAL(clearCustomConfigClicked()),
            SIGNAL(clearCustomConfigClicked()));
    connect(configFooterInfo_, SIGNAL(addCustomConfigClicked()), SLOT(onAddCustomConfigClicked()));

    widgetSearchLocations_ = new GuiLocations::WidgetLocations(this);
    widgetSearchLocations_->hide();

    updateLocationWidgetsGeometry(unscaledHeight());

    connect(widgetAllLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(widgetAllLocations_, SIGNAL(clickedOnPremiumStarCity()), SIGNAL(clickedOnPremiumStarCity()));
    connect(widgetConfiguredLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(widgetStaticIpsLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(widgetFavoriteLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(widgetSearchLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));

    connect(widgetAllLocations_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(widgetConfiguredLocations_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(widgetStaticIpsLocations_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(widgetFavoriteLocations_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(widgetSearchLocations_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));

    widgetAllLocations_->setModel(locationsModel->getAllLocationsModel());
    widgetConfiguredLocations_->setModel(locationsModel->getConfiguredLocationsModel());
    widgetStaticIpsLocations_->setModel(locationsModel->getStaticIpsLocationsModel());
    widgetFavoriteLocations_->setModel(locationsModel->getFavoriteLocationsModel());
    widgetSearchLocations_->setModel(locationsModel->getAllLocationsModel());

    searchTypingDelayTimer_.setSingleShot(true);
    searchTypingDelayTimer_.setInterval(100);
    connect(&searchTypingDelayTimer_, SIGNAL(timeout()), SLOT(onSearchTypingDelayTimerTimeout()));

    delayShowIconsTimer_.setSingleShot(true);
    delayShowIconsTimer_.setInterval(SEARCH_BUTTON_POS_ANIMATION_DURATION+100);
    connect(&delayShowIconsTimer_, SIGNAL(timeout()), SLOT(onDelayShowIconsTimerTimeout()));

    connect(locationsModel, SIGNAL(deviceNameChanged(QString)), SLOT(onDeviceNameChanged(QString)));

    updateCustomConfigsEmptyListVisibility();
}

void LocationsTab::setCountVisibleItemSlots(int cnt)
{
    if (cnt != countOfVisibleItemSlots_)
    {
        countOfVisibleItemSlots_ = cnt;
        widgetAllLocations_->setCountViewportItems(countOfVisibleItemSlots_);
        widgetConfiguredLocations_->setCountViewportItems(countOfVisibleItemSlots_-1);
        widgetStaticIpsLocations_->setCountViewportItems(countOfVisibleItemSlots_-1);
        widgetFavoriteLocations_->setCountViewportItems(countOfVisibleItemSlots_);
        widgetSearchLocations_->setCountViewportItems(countOfVisibleItemSlots_);
        updateRibbonVisibility();
        updateLocationWidgetsGeometry(unscaledHeight());
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
    Q_UNUSED(event);

    if (isWhiteAnimationActive())
    {
        update();
    }

    // cover no background between ribbon and bottom handle (static and config)
    QPainter painter(this);
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, WidgetLocationsSizes::instance().getBackgroundColor());

    // qDebug() << "LocationsTab::paintEvent - geo: " << geometry();

    drawTabRegion(painter, QRect(0, 0, width(), TOP_TAB_HEIGHT * G_SCALE));

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
                rectHoverEnter(rcAllLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "All"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcConfiguredLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_CONFIGURED_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcConfiguredLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Configured"), 9 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcStaticIpsLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_STATIC_IPS_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcStaticIpsLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Static IPs"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcFavoriteLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = LOCATION_TAB_FAVORITE_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcFavoriteLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Favourites"), 8 * G_SCALE, -5 * G_SCALE);
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

void LocationsTab::changeTab(LocationTabEnum newTab, bool animateChange)
{
    lastTab_ = curTab_;
    curTab_ = newTab;

    updateTabIconRects();

    int endWhiteLinePos;
    if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS)
    {
        endWhiteLinePos = rcConfiguredLocationsIcon_.center().x();
        onClickConfiguredLocations();
        widgetSearchLocations_->setFilterString("");
    }
    else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS)
    {
        endWhiteLinePos = rcStaticIpsLocationsIcon_.center().x();
        onClickStaticIpsLocations();
        widgetSearchLocations_->setFilterString("");
    }
    else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS)
    {
        endWhiteLinePos = rcFavoriteLocationsIcon_.center().x();
        onClickFavoriteLocations();
        widgetSearchLocations_->setFilterString("");
    }
    else if (curTab_ == LOCATION_TAB_ALL_LOCATIONS)
    {
        endWhiteLinePos = rcAllLocationsIcon_.center().x();
        onClickAllLocations();
        widgetSearchLocations_->setFilterString("");
    }
    else if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS)
    {
        // no line movement for search locations tab, it is hidden
        endWhiteLinePos = curWhiteLinePos_;
        onClickSearchLocations();
    }
    else
    {
        endWhiteLinePos = 0;
        Q_ASSERT(false);
    }

    if (animateChange)
    {
        whiteLineAnimation_.stop();
        whiteLineAnimation_.setStartValue(curWhiteLinePos_);
        whiteLineAnimation_.setEndValue(endWhiteLinePos + 2);
        whiteLineAnimation_.setDuration(ANIMATION_DURATION);
        whiteLineAnimation_.start();
    }
    else
    {
        curWhiteLinePos_ = endWhiteLinePos+2;
    }

    update();
}

void LocationsTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (curTabMouseOver_ != LOCATION_TAB_NONE &&  curTabMouseOver_ != curTab_ && tabPress_ == curTabMouseOver_)
        {
            // Currently this should only handle non-search icons
            // qDebug() << "LocationsTab::MouseReleaseEvent";
            changeTab(curTabMouseOver_);
        }
    }
}

void LocationsTab::leaveEvent(QEvent *event)
{
    curTabMouseOver_ = LOCATION_TAB_NONE;
    setArrowCursor();
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
    QWidget::leaveEvent(event);
}


void LocationsTab::keyReleaseEvent(QKeyEvent *event)
{
    qDebug() << "LocationsTab::keyReleaseEvent";
//    if (event->key() == Qt::Key_Escape)
//    {
//        if (searchLineEdit_->text() == "")
//        {
//            onSearchCancelButtonClicked();
//            event->accept();
//        }
//        else
//        {
//            searchLineEdit_->setText("");
//            event->accept();
//        }
//    }

    QWidget::keyReleaseEvent(event);
}

bool LocationsTab::eventFilter(QObject *object, QEvent *event)
{
    // qDebug() << "LocationsTab::eventFilter";
    if (object == searchLineEdit_)
    {
        if (event->type() == QEvent::KeyRelease)
        {
            // qDebug() << "Filtered KeyRelease from search line edit";
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            {
                passEventToLocationWidget(keyEvent);
            }
            else if (keyEvent->key() == Qt::Key_Down)
            {
                passEventToLocationWidget(keyEvent);
            }
            else if (keyEvent->key() == Qt::Key_Up)
            {
                passEventToLocationWidget(keyEvent);
            }
            else if (keyEvent->key() == Qt::Key_Escape)
            {
                if (searchLineEdit_->text() == "")
                {
                    onSearchCancelButtonClicked();
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
                return true;
            }
        }
    }
    return QWidget::eventFilter(object, event);
}

void LocationsTab::onClickAllLocations()
{
    widgetAllLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetSearchLocations_->hide();
    widgetAllLocations_->show();
    widgetAllLocations_->raise();
}

void LocationsTab::onClickConfiguredLocations()
{
    widgetConfiguredLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
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
    widgetStaticIpsLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
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
    widgetFavoriteLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
    widgetAllLocations_->hide();
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetSearchLocations_->hide();
    widgetFavoriteLocations_->show();
    widgetFavoriteLocations_->raise();
}

void LocationsTab::onClickSearchLocations()
{
    widgetSearchLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetAllLocations_->hide();
    widgetSearchLocations_->show();
    widgetSearchLocations_->raise();
}

void LocationsTab::switchToSearchAndRestoreAccentedItem()
{
    LocationID previousSelectedLocId;
    IWidgetLocationsInfo *locWidget = locationWidgetByEnum(LOCATION_TAB_SEARCH_LOCATIONS);
    if (locWidget)
    {
        previousSelectedLocId = locWidget->accentedItemLocationId();
    }

    onSearchButtonClicked(); // this will handle changeTab(..)

    if (previousSelectedLocId.isValid())
    {
        if (locWidget->cursorInViewport())
        {
            locWidget->centerCursorOnItem(previousSelectedLocId);
        }
    }
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
    checkCustomConfigPathAccessRights_ = true;
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
    // qDebug() << "Search clicked";

    if (showAllTabs_)
    {
        searchButton_->setEnabled(false);

        changeTab(LOCATION_TAB_SEARCH_LOCATIONS);

        searchTabSelected_ = true;
        update();

        QTimer::singleShot(100, [this]()
        {
            searchButtonPosAnimation_.stop();
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
        });
    }
}

void LocationsTab::onSearchCancelButtonClicked()
{
    // qDebug() << "Search cancel clicked";
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
    // qDebug() << "Setting filter with: " << filterText_;
    widgetSearchLocations_->setFilterString(filterText_);
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

IWidgetLocationsInfo *LocationsTab::currentWidgetLocations()
{
    return locationWidgetByEnum(curTab_);
}

IWidgetLocationsInfo *LocationsTab::locationWidgetByEnum(LocationsTab::LocationTabEnum tabEnum)
{
    if (tabEnum == LOCATION_TAB_ALL_LOCATIONS)        return widgetAllLocations_;
    if (tabEnum == LOCATION_TAB_FAVORITE_LOCATIONS)   return widgetFavoriteLocations_;
    if (tabEnum == LOCATION_TAB_STATIC_IPS_LOCATIONS) return widgetStaticIpsLocations_;
    if (tabEnum == LOCATION_TAB_CONFIGURED_LOCATIONS) return widgetConfiguredLocations_;
    if (tabEnum == LOCATION_TAB_SEARCH_LOCATIONS)     return widgetSearchLocations_;
    return nullptr;
}

void LocationsTab::drawTabRegion(QPainter &painter, const QRect &rc)
{
    painter.fillRect(rc, QBrush(tabBackgroundColor_));

    if (!searchTabSelected_)
    {
        // draw white line
        drawBottomLine(painter, rc.left(), rc.right(), rc.bottom(), curWhiteLinePos_);

        // Draw Icons
        {
            IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/CONFIG_ICON");
            painter.setOpacity(curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
            p->draw(rcConfiguredLocationsIcon_.left(), rcConfiguredLocationsIcon_.top(), &painter);
        }
        if (showAllTabs_)
        {
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/STATIC_IP_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcStaticIpsLocationsIcon_.left(), rcStaticIpsLocationsIcon_.top(), &painter);
            }
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcFavoriteLocationsIcon_.left(), rcFavoriteLocationsIcon_.top(), &painter);
            }
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/ALL_LOCATION_ICON");
                painter.setOpacity(curTab_ == LOCATION_TAB_ALL_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcAllLocationsIcon_.left(), rcAllLocationsIcon_.top(), &painter);
            }
        }
    }
}

void LocationsTab::drawBottomLine(QPainter &painter, int left, int right, int bottom, int whiteLinePos)
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
    IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
    if (curWidgetLoc != nullptr)
    {
        if (curWidgetLoc->hasAccentItem())
        {
            curWidgetLoc->handleKeyEvent(event);
        }
        else
        {
            curWidgetLoc->accentFirstItem();
        }
    }
}

void LocationsTab::handleKeyReleaseEvent(QKeyEvent *event)
{
    // qDebug() << "LocationsTab::handleKeyReleaseEvent";
    TooltipController::instance().hideAllTooltips();

    if (event->key() == Qt::Key_Right)
    {
        int curTabInt = static_cast<int>(curTab_);
        if (curTabInt < LOCATION_TAB_LAST)
        {
            curTabInt++;

            if (curTabInt == LOCATION_TAB_SEARCH_LOCATIONS)
            {
                switchToSearchAndRestoreAccentedItem();
            }
            else
            {
                changeTab(static_cast<LocationTabEnum>(curTabInt));

                IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
                if (curWidgetLoc != nullptr)
                {
                    if (curWidgetLoc->cursorInViewport())
                    {
                        curWidgetLoc->centerCursorOnAccentedItem();
                    }
                }
            }

        }
    }
    else if (event->key() == Qt::Key_Left)
    {
        int curTabInt = static_cast<int>(curTab_);
        if (curTabInt > LOCATION_TAB_FIRST)
        {
            curTabInt--;
            if (curTabInt == LOCATION_TAB_SEARCH_LOCATIONS)
            {
                // don't think this can happen
                switchToSearchAndRestoreAccentedItem();
            }
            else
            {
                if (curTabInt == LOCATION_TAB_CONFIGURED_LOCATIONS)
                {
                    onSearchCancelButtonClicked(); // handles changeTab(..)
                }
                else
                {
                    changeTab(static_cast<LocationTabEnum>(curTabInt));
                }

                IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
                if (curWidgetLoc != nullptr)
                {
                    if (curWidgetLoc->cursorInViewport())
                    {
                        curWidgetLoc->centerCursorOnAccentedItem();
                    }
                }
            }
        }
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
    }
    else if (event->key() == Qt::Key_Up ||
             event->key() == Qt::Key_Down ||
             event->key() == Qt::Key_Return ||
             event->key() == Qt::Key_Enter )
    {
        passEventToLocationWidget(event);
    }
}

void LocationsTab::handleKeyPressEvent(QKeyEvent *event)
{
    // let the release handle these
    if (event->key() == Qt::Key_Right  ||
        event->key() == Qt::Key_Left   ||
        event->key() == Qt::Key_Up     ||
        event->key() == Qt::Key_Down   ||
        event->key() == Qt::Key_Tab    ||
        event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter )
    {
        return;
    }

    // block non-chars
    if ((event->key() & Qt::Key_Escape) == Qt::Key_Escape)
    {
        return;
    }

    // set locations tab to search screen
    if (curTab_ != LOCATION_TAB_SEARCH_LOCATIONS)
    {
        if (event->key() != Qt::Key_Space) // prevent space from triggering search when intention is to close locations tray
        {
            switchToSearchAndRestoreAccentedItem();

            // Adding text to search text must be handled in the keyPress instead of keyRelease because
            // during focus transition from locationsTab -> searchLineEdit: mainWindow no longer fires keyReleaseEvents
            // while searchLineEdit has not yet picked up the focus (it uses keyPressEvents too)
            // qDebug() << "Appending text: " << event->text();
            searchLineEdit_->appendText(event->text());
            searchLineEdit_->setFocus();
        }
    }
    else
    {
        searchLineEdit_->appendText(event->text());
        searchLineEdit_->setFocus();
    }
}

void LocationsTab::updateTabIconRects()
{
    searchButton_->move(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
    searchCancelButton_->move(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

    int totalWidthForFirst4TabsWithSpacing = LAST_TAB_ICON_POS_X - FIRST_TAB_ICON_POS_X;
    int iconWidths = 16 + 16 + 16 + 18;
    int totalSpacing = totalWidthForFirst4TabsWithSpacing - iconWidths;
    int eachSpacing = totalSpacing/4;

    int secondPos = FIRST_TAB_ICON_POS_X + eachSpacing + 16;
    int thirdPos = secondPos + eachSpacing + 16;
    int fourthPos = thirdPos + eachSpacing + 16;
    rcAllLocationsIcon_.       setRect(FIRST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16 * G_SCALE, 16 * G_SCALE);
    rcFavoriteLocationsIcon_.  setRect(secondPos * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16 * G_SCALE, 14 * G_SCALE);
    rcStaticIpsLocationsIcon_. setRect(thirdPos * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16 * G_SCALE, 16 * G_SCALE);

    if (showAllTabs_)
    {
        rcConfiguredLocationsIcon_.setRect(fourthPos * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 18 * G_SCALE, 16 * G_SCALE);
        searchButton_->show();
    }
    else
    {
        rcConfiguredLocationsIcon_.setRect(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 18 * G_SCALE, 16 * G_SCALE);
        searchButton_->hide();
    }
}

void LocationsTab::updateIconRectsAndLine()
{
    updateTabIconRects();

    // update white line pos
    if      (currentWidgetLocations() == widgetAllLocations_)        curWhiteLinePos_ = (rcAllLocationsIcon_.center().x() + 1*G_SCALE)        ;
    else if (currentWidgetLocations() == widgetConfiguredLocations_) curWhiteLinePos_ = (rcConfiguredLocationsIcon_.center().x() + 1*G_SCALE) ;
    else if (currentWidgetLocations() == widgetStaticIpsLocations_)  curWhiteLinePos_ = (rcStaticIpsLocationsIcon_.center().x() + 1*G_SCALE)  ;
    else if (currentWidgetLocations() == widgetFavoriteLocations_)   curWhiteLinePos_ = (rcFavoriteLocationsIcon_.center().x() + 1*G_SCALE)   ;
    // not drawing line for widgetSearchLocations_

    update();
}

void LocationsTab::updateLocationWidgetsGeometry(int newHeight)
{
    const int kRibbonHeight = isRibbonVisible_ ? 48 : 0;

    currentLocationListHeight_ = newHeight;

    // qDebug() << "Setting height of all location widgets: " << newHeight * G_SCALE;
    widgetAllLocations_->setGeometry(
        0, TOP_TAB_HEIGHT * G_SCALE, WINDOW_WIDTH * G_SCALE, newHeight * G_SCALE);
    widgetFavoriteLocations_->setGeometry(
        0, TOP_TAB_HEIGHT * G_SCALE, WINDOW_WIDTH * G_SCALE, newHeight * G_SCALE);
    widgetStaticIpsLocations_->setGeometry(
        0, TOP_TAB_HEIGHT * G_SCALE, WINDOW_WIDTH * G_SCALE, (newHeight - kRibbonHeight) * G_SCALE);
    widgetConfiguredLocations_->setGeometry(
        0, TOP_TAB_HEIGHT * G_SCALE, WINDOW_WIDTH * G_SCALE, (newHeight - kRibbonHeight)* G_SCALE);
    widgetSearchLocations_->setGeometry(
        0, TOP_TAB_HEIGHT * G_SCALE, WINDOW_WIDTH * G_SCALE, newHeight * G_SCALE);

//    widgetFavoriteLocations_->setSize(WINDOW_WIDTH, newHeight);
//    widgetStaticIpsLocations_->setSize(WINDOW_WIDTH, newHeight - kRibbonHeight );
//    widgetConfiguredLocations_->setSize(WINDOW_WIDTH, newHeight - kRibbonHeight);

    staticIPDeviceInfo_->setGeometry(
        0, newHeight * G_SCALE, WINDOW_WIDTH * G_SCALE, kRibbonHeight * G_SCALE);
    configFooterInfo_->setGeometry(
        0, newHeight * G_SCALE, WINDOW_WIDTH * G_SCALE, kRibbonHeight * G_SCALE);

    updateScaling();
}

void LocationsTab::updateScaling()
{
    widgetAllLocations_->       updateScaling();
    widgetFavoriteLocations_->  updateScaling();
    widgetStaticIpsLocations_-> updateScaling();
    widgetConfiguredLocations_->updateScaling();
    widgetSearchLocations_->    updateScaling();
    configFooterInfo_->         updateScaling();

    searchButton_->setGeometry(searchButtonPosUnscaled_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16*G_SCALE, 16*G_SCALE);
    searchCancelButton_->setGeometry(LAST_TAB_ICON_POS_X * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16*G_SCALE, 16*G_SCALE);
    searchLineEdit_->setFont(*FontManager::instance().getFont(14, false));
    searchLineEdit_->setGeometry((FIRST_TAB_ICON_POS_X + 32)*G_SCALE, 2*G_SCALE,
                                 (LAST_TAB_ICON_POS_X - FIRST_TAB_ICON_POS_X - 32) *G_SCALE,
                                 40*G_SCALE);
}

void LocationsTab::updateLanguage()
{
    widgetFavoriteLocations_->setEmptyListDisplayText(tr("Nothing to see here"));
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

LocationsTab::LocationTabEnum LocationsTab::currentTab()
{
    return curTab_;
}

int LocationsTab::unscaledHeight()
{
    return 50 * countOfVisibleItemSlots_ /* - 1 */; // TODO: still need -1 ?
}

void LocationsTab::setLatencyDisplay(ProtoTypes::LatencyDisplayType l)
{
    bool isShowLatencyInMs = (l == ProtoTypes::LATENCY_DISPLAY_MS);
    if (isShowLatencyInMs != widgetAllLocations_->isShowLatencyInMs())
    {
        widgetAllLocations_       ->setShowLatencyInMs(isShowLatencyInMs);
        widgetConfiguredLocations_->setShowLatencyInMs(isShowLatencyInMs);
        widgetStaticIpsLocations_ ->setShowLatencyInMs(isShowLatencyInMs);
        widgetFavoriteLocations_  ->setShowLatencyInMs(isShowLatencyInMs);
        widgetSearchLocations_    ->setShowLatencyInMs(isShowLatencyInMs);
    }
}

void LocationsTab::setCustomConfigsPath(QString path)
{
    configFooterInfo_->setText(path);
    updateCustomConfigsEmptyListVisibility();
    updateRibbonVisibility();

    if (checkCustomConfigPathAccessRights_) {
        WriteAccessRightsChecker checker(path);
        if (checker.isWriteable()) {
            const QString desc = tr(
                "The selected directory is writeable for non-privileged users.\n"
                "Custom configs in this directory may pose a potential security risk.\n"
                "Directory: \"%1\"")
                .arg(path);
            QMessageBox::warning(g_mainWindow, tr("Windscribe"), desc);
        }
        checkCustomConfigPathAccessRights_ = false;
    }
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
    Q_ASSERT(configFooterInfo_ != nullptr);
    if (configFooterInfo_->text().isEmpty()) {
        widgetConfiguredLocations_->setEmptyListDisplayText(
            tr("Choose the directory that contains custom configs you wish to display here"), 160);
        widgetConfiguredLocations_->setEmptyListButtonText(tr("Choose"));
    } else {
        widgetConfiguredLocations_->setEmptyListDisplayText(
            tr("The selected directory contains no custom configs"), 160);
        widgetConfiguredLocations_->setEmptyListButtonText(QString());
    }
}

void LocationsTab::updateRibbonVisibility()
{
    auto *current_widget_locations = currentWidgetLocations();
    const bool is_location_list_empty = current_widget_locations->countViewportItems() <= 1;

    isRibbonVisible_ = false;

    if (current_widget_locations == widgetStaticIpsLocations_)
    {
        configFooterInfo_->hide();
        if (is_location_list_empty) {
            staticIPDeviceInfo_->hide();
        } else {
            staticIPDeviceInfo_->show();
            staticIPDeviceInfo_->raise();
            isRibbonVisible_ = true;
        }
    }
    else if (current_widget_locations == widgetConfiguredLocations_)
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

} // namespace GuiLocations

