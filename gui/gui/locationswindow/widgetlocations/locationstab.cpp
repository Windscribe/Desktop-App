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

#include <QDebug>

extern QWidget *g_mainWindow;

namespace GuiLocations {


LocationsTab::LocationsTab(QWidget *parent, LocationsModel *locationsModel) : QWidget(parent),
    curTab_(CUR_TAB_ALL_LOCATIONS),
    tabPress_(CUR_TAB_NONE),
    searchTabSelected_(false),
    curTabMouseOver_(CUR_TAB_NONE),
    checkCustomConfigPathAccessRights_(false),
    countOfVisibleItemSlots_(7),
    currentLocationListHeight_(0),
    isRibbonVisible_(false),
    showAllTabs_(true),
    backgroundColor_(14, 25, 38)
{
    setMouseTracking(true);
    curCursorShape_ = Qt::ArrowCursor;

    searchButtonPos_ = LAST_TAB_ICON_POS_X;
    searchButton_ = new CommonWidgets::IconButtonWidget("locations/SEARCH_ICON", this);
    searchButton_->setUnhoverHoverOpacity(TAB_OPACITY_DIM, TAB_OPACITY_DIM);
    connect(searchButton_, SIGNAL(clicked()), SLOT(onSearchButtonClicked()));
    searchButton_->move(searchButtonPos_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);

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
    connect(searchLineEdit_, SIGNAL(textChanged(QString)), SLOT(onSearchLineEditTextChanged(QString)));
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
    int newHeight = 50 * countOfVisibleItemSlots_ - 1;

    widgetSearchLocations_ = new GuiLocations::SearchWidgetLocations(this);
    widgetSearchLocations_->hide();

    updateLocationWidgetsGeometry(newHeight);

    connect(widgetAllLocations_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
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

    connect(locationsModel, SIGNAL(deviceNameChanged(QString)), SLOT(onDeviceNameChanged(QString)));

    updateCustomConfigsEmptyListVisibility();
}

int LocationsTab::setCountVisibleItemSlots(int cnt)
{
    if (cnt != countOfVisibleItemSlots_)
    {
        countOfVisibleItemSlots_ = cnt;
        widgetAllLocations_->setCountAvailableItemSlots(countOfVisibleItemSlots_);
        widgetConfiguredLocations_->setCountAvailableItemSlots(countOfVisibleItemSlots_-1);
        widgetStaticIpsLocations_->setCountAvailableItemSlots(countOfVisibleItemSlots_-1);
        widgetFavoriteLocations_->setCountAvailableItemSlots(countOfVisibleItemSlots_);
        // widgetSearchLocations_->setCountAvailableItemSlots(countOfVisibleItemSlots_);
        updateRibbonVisibility();

        const int newHeight = 50 * countOfVisibleItemSlots_ - 1;
        updateLocationWidgetsGeometry(newHeight);
        return newHeight ;
    }
    else
    {
        return (50 * countOfVisibleItemSlots_ - 1);
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
        changeTab(CUR_TAB_CONFIGURED_LOCATIONS);
    }
    else
    {
        showAllTabs_ = true;
        changeTab(CUR_TAB_ALL_LOCATIONS);
    }
}

void LocationsTab::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (isWhiteAnimationActive())
    {
        update();
    }

    QPainter painter(this);
    drawTab(painter, QRect(0, 0, width(), TOP_TAB_HEIGHT * G_SCALE));

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
                curTabMouseOver_ = CUR_TAB_ALL_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcAllLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "All"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcConfiguredLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = CUR_TAB_CONFIGURED_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcConfiguredLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Configured"), 9 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcStaticIpsLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = CUR_TAB_STATIC_IPS_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcStaticIpsLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Static IPs"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else if (rcFavoriteLocationsIcon_.adjusted(-addMargin, -addMargin, addMargin, addMargin).contains(pt))
            {
                curTabMouseOver_ = CUR_TAB_FAVORITE_LOCATIONS;
                setPointingHandCursor();
                rectHoverEnter(rcFavoriteLocationsIcon_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Favourites"), 8 * G_SCALE, -5 * G_SCALE);
            }
            else
            {
                curTabMouseOver_ = CUR_TAB_NONE;
                setArrowCursor();

                TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
            }
        }
        else
        {
            curTabMouseOver_ = CUR_TAB_NONE;
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
        if (curTabMouseOver_ != CUR_TAB_NONE)
        {
            tabPress_ = curTabMouseOver_;
        }
    }
}

void LocationsTab::changeTab(CurTabEnum newTab)
{
    lastTab_ = curTab_;
    curTab_ = newTab;

    updateTabIconRects();

    int endWhiteLinePos;
    if (curTab_ == CUR_TAB_CONFIGURED_LOCATIONS)
    {
        endWhiteLinePos = rcConfiguredLocationsIcon_.center().x();
        onClickConfiguredLocations();
    }
    else if (curTab_ == CUR_TAB_STATIC_IPS_LOCATIONS)
    {
        endWhiteLinePos = rcStaticIpsLocationsIcon_.center().x();
        onClickStaticIpsLocations();
    }
    else if (curTab_ == CUR_TAB_FAVORITE_LOCATIONS)
    {
        endWhiteLinePos = rcFavoriteLocationsIcon_.center().x();
        onClickFavoriteLocations();
    }
    else if (curTab_ == CUR_TAB_ALL_LOCATIONS)
    {
        endWhiteLinePos = rcAllLocationsIcon_.center().x();
        onClickAllLocations();
    }
    else if (curTab_ == CUR_TAB_SEARCH_LOCATIONS)
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

    whiteLineAnimation_.stop();
    whiteLineAnimation_.setStartValue(curWhiteLinePos_);
    whiteLineAnimation_.setEndValue(endWhiteLinePos + 2);
    whiteLineAnimation_.setDuration(ANIMATION_DURATION);
    whiteLineAnimation_.start();

    update();
}

void LocationsTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (curTabMouseOver_ != CUR_TAB_NONE &&  curTabMouseOver_ != curTab_ && tabPress_ == curTabMouseOver_)
        {
            // Currently this should only handle non-search icons
            // qDebug() << "LocationsTab::MouseReleaseEvent";
            changeTab(curTabMouseOver_);
        }
    }
}

void LocationsTab::leaveEvent(QEvent *event)
{
    curTabMouseOver_ = CUR_TAB_NONE;
    setArrowCursor();
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_TAB_INFO);
    QWidget::leaveEvent(event);
}


void LocationsTab::keyReleaseEvent(QKeyEvent *event)
{
    // qDebug() << "LocationsTab::keyReleaseEvent";
    if (event->key() == Qt::Key_Escape)
    {
        if (searchLineEdit_->text() == "")
        {
            onSearchCancelButtonClicked();
            event->accept();
        }
        else
        {
            searchLineEdit_->setText("");
            event->accept();
        }
    }
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
    //widgetSearchLocations_->startAnimationWithPixmap(this->grab(QRect(0, TOP_TAB_HEIGHT* G_SCALE, width(), height() - TOP_TAB_HEIGHT* G_SCALE)));
    widgetConfiguredLocations_->hide();
    widgetStaticIpsLocations_->hide();
    widgetFavoriteLocations_->hide();
    widgetAllLocations_->hide();
    widgetSearchLocations_->show();
    widgetSearchLocations_->raise();
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

void LocationsTab::onSearchButtonClicked()
{
    // qDebug() << "Search clicked";

    if (showAllTabs_)
    {
        searchButton_->setEnabled(false);

        changeTab(CUR_TAB_SEARCH_LOCATIONS);

        searchTabSelected_ = true;
        update();

        QTimer::singleShot(100, [this]()
        {
            searchButtonPosAnimation_.setDirection(QAbstractAnimation::Backward);
            searchButtonPosAnimation_.start();

            searchCancelButton_->show();
            searchLineEdit_->show();
            searchLineEdit_->setFocus();
        });
    }
}

void LocationsTab::onSearchCancelButtonClicked()
{
    // qDebug() << "Search cancel clicked";

    if (searchTabSelected_)
    {
        searchButton_->setEnabled(true);
        searchButtonPosAnimation_.setDirection(QAbstractAnimation::Forward);
        searchButtonPosAnimation_.start();

        searchCancelButton_->hide();
        searchLineEdit_->hide();

        changeTab(lastTab_);

        // let animation finish before showing all tabs
        QTimer::singleShot(SEARCH_BUTTON_POS_ANIMATION_DURATION, [this](){
            searchTabSelected_ = false;
            update();
        });
    }
}

void LocationsTab::onSearchButtonPosAnimationValueChanged(const QVariant &value)
{
    searchButtonPos_ = value.toInt();
    searchButton_->move(searchButtonPos_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
}

void LocationsTab::onSearchLineEditTextChanged(QString text)
{
    // TODO: update view
}

IWidgetLocationsInfo *LocationsTab::currentWidgetLocations()
{
    if (curTab_ == CUR_TAB_ALL_LOCATIONS)        return widgetAllLocations_;
    if (curTab_ == CUR_TAB_FAVORITE_LOCATIONS)   return widgetFavoriteLocations_;
    if (curTab_ == CUR_TAB_STATIC_IPS_LOCATIONS) return widgetStaticIpsLocations_;
    if (curTab_ == CUR_TAB_CONFIGURED_LOCATIONS) return widgetConfiguredLocations_;
    if (curTab_ == CUR_TAB_SEARCH_LOCATIONS)     return widgetSearchLocations_;
    return nullptr;
}

void LocationsTab::drawTab(QPainter &painter, const QRect &rc)
{
    painter.fillRect(rc, QBrush(backgroundColor_));

    if (!searchTabSelected_)
    {
        // draw white line
        drawBottomLine(painter, rc.left(), rc.right(), rc.bottom(), curWhiteLinePos_);

        // Draw Icons
        {
            IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/CONFIG_ICON");
            painter.setOpacity(curTab_ == CUR_TAB_CONFIGURED_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
            p->draw(rcConfiguredLocationsIcon_.left(), rcConfiguredLocationsIcon_.top(), &painter);
        }
        if (showAllTabs_)
        {
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/STATIC_IP_ICON");
                painter.setOpacity(curTab_ == CUR_TAB_STATIC_IPS_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcStaticIpsLocationsIcon_.left(), rcStaticIpsLocationsIcon_.top(), &painter);
            }
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON");
                painter.setOpacity(curTab_ == CUR_TAB_FAVORITE_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcFavoriteLocationsIcon_.left(), rcFavoriteLocationsIcon_.top(), &painter);
            }
            {
                IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/ALL_LOCATION_ICON");
                painter.setOpacity(curTab_ == CUR_TAB_ALL_LOCATIONS ? 1.0 : TAB_OPACITY_DIM);
                p->draw(rcAllLocationsIcon_.left(), rcAllLocationsIcon_.top(), &painter);
            }
        }
    }
}

void LocationsTab::drawBottomLine(QPainter &painter, int left, int right, int bottom, int whiteLinePos)
{
    painter.fillRect(QRect(left, bottom-G_SCALE*2 + 1, right-left, G_SCALE*2), QBrush(QColor(3, 9, 28)));
    // draw white line
    {
        painter.fillRect(QRect((whiteLinePos - WHITE_LINE_WIDTH / 2 *G_SCALE), bottom-G_SCALE*2 + 1, WHITE_LINE_WIDTH*G_SCALE, G_SCALE*2), QBrush(Qt::white));
    }
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

void LocationsTab::handleKeyReleaseEvent(QKeyEvent *event)
{
    // TODO: fix key navigation
    // * left/right into/outof search
    // * up/down in search

    if (event->key() == Qt::Key_Right)
    {
        int curTabInt = static_cast<int>(curTab_);
        if (curTabInt < CUR_TAB_LAST)
        {
            curTabInt++;
            changeTab(static_cast<CurTabEnum>(curTabInt));

            IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
            if (curWidgetLoc != nullptr)
            {
                if (curWidgetLoc->cursorInViewport())
                {
                    curWidgetLoc->centerCursorOnSelectedItem();
                }
            }
        }
    }
    else if (event->key() == Qt::Key_Left)
    {
        int curTabInt = static_cast<int>(curTab_);
        if (curTabInt > CUR_TAB_FIRST)
        {
            curTabInt--;
            changeTab(static_cast<CurTabEnum>(curTabInt));

            IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
            if (curWidgetLoc != nullptr)
            {
                if (curWidgetLoc->cursorInViewport())
                {
                    curWidgetLoc->centerCursorOnSelectedItem();
                }
            }
        }
    }
    else
    {
        IWidgetLocationsInfo * curWidgetLoc = currentWidgetLocations();
        if (curWidgetLoc != nullptr)
        {
            if (curWidgetLoc->hasSelection())
            {
                curWidgetLoc->handleKeyEvent(event);
            }
            else
            {
                curWidgetLoc->setFirstSelected();
            }
        }
    }
}

void LocationsTab::updateTabIconRects()
{
    searchButton_->move(searchButtonPos_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE);
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

    widgetAllLocations_->setSize(WINDOW_WIDTH, newHeight);
    widgetFavoriteLocations_->setSize(WINDOW_WIDTH, newHeight);
    widgetStaticIpsLocations_->setSize(WINDOW_WIDTH, newHeight - kRibbonHeight );
    widgetConfiguredLocations_->setSize(WINDOW_WIDTH, newHeight - kRibbonHeight);
    // widgetSearchLocations_->setSize(WINDOW_WIDTH, newHeight);


    staticIPDeviceInfo_->setGeometry(
        0, (newHeight - 2) * G_SCALE, WINDOW_WIDTH * G_SCALE, kRibbonHeight * G_SCALE);
    configFooterInfo_->setGeometry(
        0, (newHeight - 2) * G_SCALE, WINDOW_WIDTH * G_SCALE, kRibbonHeight * G_SCALE);

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

    searchButton_->setGeometry(searchButtonPos_ * G_SCALE, TOP_TAB_MARGIN * G_SCALE, 16*G_SCALE, 16*G_SCALE);
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
    const bool is_location_list_empty = current_widget_locations->countVisibleItems() <= 1;

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

