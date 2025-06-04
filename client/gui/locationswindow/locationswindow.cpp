#include "locationswindow.h"

#include <QApplication>
#include <QPainter>
#include <QtMath>

#include "backend/persistentstate.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"

#include <QDebug>

LocationsWindow::LocationsWindow(QWidget *parent, Preferences *preferences, gui_locations::LocationsModelManager *locationsModelManager) : QWidget(parent)
  , locationsTabHeightUnscaled_(LOCATIONS_TAB_HEIGHT_INIT)
  , bDragPressed_(false)
{
    setMouseTracking(true);

    locationsTab_ = new GuiLocations::LocationsTab(this, preferences, locationsModelManager);
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, qCeil(locationsTabHeightUnscaled_ * G_SCALE));

    connect(locationsTab_, &GuiLocations::LocationsTab::selected, this, &LocationsWindow::selected);
    connect(locationsTab_, &GuiLocations::LocationsTab::clickedOnPremiumStarCity, this, &LocationsWindow::clickedOnPremiumStarCity);
    connect(locationsTab_, &GuiLocations::LocationsTab::addStaticIpClicked, this, &LocationsWindow::addStaticIpClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::clearCustomConfigClicked, this, &LocationsWindow::clearCustomConfigClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::addCustomConfigClicked, this, &LocationsWindow::addCustomConfigClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::upgradeBannerClicked, this, &LocationsWindow::upgradeBannerClicked);

    connect(preferences, &Preferences::appSkinChanged, this, &LocationsWindow::onAppSkinChanged);

    setCountVisibleItemSlots(PersistentState::instance().countVisibleLocations());
}

int LocationsWindow::tabAndFooterHeight() const
{
    return (locationsTabHeightUnscaled_ + FOOTER_HEIGHT);
}

void LocationsWindow::setCountVisibleItemSlots(int cnt)
{
    locationsTab_->setCountVisibleItemSlots(cnt);
    // Previously there were issues directly grabbing locationsTab height... keeping a cache somehow helped. Not sure if the original issue persists
    locationsTabHeightUnscaled_ = locationsTab_->unscaledHeightOfItemViewport() + locationsTab_->headerHeight();
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));
    PersistentState::instance().setCountVisibleLocations(getCountVisibleItems());
    emit heightChanged();
}

int LocationsWindow::getCountVisibleItems()
{
    return locationsTab_->getCountVisibleItems();
}

void LocationsWindow::updateLocationsTabGeometry()
{
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));

    locationsTab_->updateLocationWidgetsGeometry(locationsTab_->unscaledHeightOfItemViewport());
    locationsTab_->update();
}

void LocationsWindow::updateScaling()
{
    locationsTab_->updateScaling();
}

void LocationsWindow::setShowLocationLoad(bool showLocationLoad)
{
    locationsTab_->setShowLocationLoad(showLocationLoad);
}

void LocationsWindow::setCustomConfigsPath(QString path)
{
    locationsTab_->setCustomConfigsPath(path);
}

void LocationsWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // qDebug() << "LocationsWindow::paintEvent - geo: " << geometry();

    // footer background
    QColor footerColor = FontManager::instance().getLocationsFooterColor();
#ifdef Q_OS_MACOS
        p.setPen(footerColor);
        p.setBrush(footerColor);
        // draw rounded corners on mac
        p.drawRoundedRect(QRect(0, height() - FOOTER_HEIGHT * G_SCALE,
                                width(), FOOTER_HEIGHT * G_SCALE), 8, 8);
        // roundedRect leaves gap between bottom of list and footer at corners -- cover it
        p.fillRect(QRect(0, height() - FOOTER_HEIGHT * G_SCALE,
                         width(), (FOOTER_HEIGHT) * G_SCALE / 2), QBrush(footerColor));
#else
        // drawing FOOTER_HEIGHT_FULL here should make no difference since the child list will draw over anyway
        p.fillRect(QRect(0, height() - FOOTER_HEIGHT * G_SCALE, width(), FOOTER_HEIGHT * G_SCALE), QBrush(footerColor));
#endif

    // footer handle
    QRect middle(width() / 2 - 12* G_SCALE,
                 height() - (FOOTER_HEIGHT_FULL+2) * G_SCALE / 2,
                 24 * G_SCALE, 3 * G_SCALE);
    p.setOpacity(0.5);
    p.fillRect(QRect(middle.left(), middle.top(), middle.width(), 2 * G_SCALE), QBrush(Qt::white));
}

void LocationsWindow::resizeEvent(QResizeEvent *event)
{
    locationsTab_->move(0, (event->size().height() - tabAndFooterHeight() * G_SCALE));
    QWidget::resizeEvent(event);
}

void LocationsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (bDragPressed_)
    {
        int y_offs = event->globalPosition().toPoint().y() - dragPressPt_.y();

        int cnt = y_offs / (50 * G_SCALE);
        int curCntVisibleItems = dragInitialVisibleItemsCount_ + cnt;
        // cursor below drag point
        if (y_offs > 0)
        {
            if (curCntVisibleItems < getCountVisibleItems())
            {
                curCntVisibleItems++;
            }
        }
        // cursor above drag point
        else if (y_offs < 0)
        {
            if (curCntVisibleItems > getCountVisibleItems())
            {
                curCntVisibleItems--;
            }
        }

        if (curCntVisibleItems >= MIN_VISIBLE_LOCATIONS && curCntVisibleItems <= MAX_VISIBLE_LOCATIONS)
        {
            if (curCntVisibleItems != getCountVisibleItems())
            {
                setCountVisibleItemSlots(curCntVisibleItems);
            }
        }

    }
    else
    {
        QRect middle = getResizeHandleClickableRect();
        if (middle.contains(event->pos()))
        {
            setCursor(Qt::SizeVerCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
    }
    QWidget::mouseMoveEvent(event);
}

void LocationsWindow::mousePressEvent(QMouseEvent *event)
{
    QRect middle = getResizeHandleClickableRect();
    if (middle.contains(event->pos()))
    {
        bDragPressed_ = true;
        dragPressPt_ = event->globalPosition().toPoint();
        dragInitialVisibleItemsCount_ = getCountVisibleItems();

        QPoint centerBtnDrag = mapToGlobal(middle.center());
        dragInitialBtnDragCenter_ = centerBtnDrag.y();
    }
}

void LocationsWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (bDragPressed_)
    {
        bDragPressed_ = false;
    }
}

QRect LocationsWindow::getResizeHandleClickableRect()
{
    return QRect(width() / 2 - 14*G_SCALE,
                 height() - FOOTER_HEIGHT_FULL*G_SCALE / 2 - 2*G_SCALE,
                 28*G_SCALE, 8*G_SCALE);
}

void LocationsWindow::setTab(LOCATION_TAB tab)
{
    switch (tab) {
        case LOCATION_TAB_ALL_LOCATIONS:
            locationsTab_->onClickAllLocations();
            break;
        case LOCATION_TAB_CONFIGURED_LOCATIONS:
            locationsTab_->onClickConfiguredLocations();
            break;
        case LOCATION_TAB_STATIC_IPS_LOCATIONS:
            locationsTab_->onClickStaticIpsLocations();
            break;
        case LOCATION_TAB_FAVORITE_LOCATIONS:
            locationsTab_->onClickFavoriteLocations();
            break;
        case LOCATION_TAB_SEARCH_LOCATIONS:
            locationsTab_->onClickSearchLocations();
            break;
        default:
            WS_ASSERT(false);
    }
}

void LocationsWindow::onSearchFilterChanged(const QString &filter)
{
    locationsTab_->onSearchFilterChanged(filter);
}

void LocationsWindow::onLocationsKeyPressed(QKeyEvent *event)
{
    locationsTab_->onLocationsKeyPressed(event);
}

void LocationsWindow::setIsPremium(bool isPremium)
{
    locationsTab_->setIsPremium(isPremium);
}

void LocationsWindow::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    locationsTab_->setDataRemaining(bytesUsed, bytesMax);
}

void LocationsWindow::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    locationsTabHeightUnscaled_ = locationsTab_->unscaledHeightOfItemViewport() + locationsTab_->headerHeight();
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));
    emit heightChanged();
}
