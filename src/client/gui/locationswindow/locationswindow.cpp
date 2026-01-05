#include "locationswindow.h"

#include <QApplication>
#include <QPainter>
#include <QtMath>

#include "backend/persistentstate.h"
#include "commongraphics/commongraphics.h"
#include "commongraphics/footerbackground.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "languagecontroller.h"

#include <QDebug>

namespace {

class BorderOverlay : public QWidget
{
public:
    explicit BorderOverlay(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        QSharedPointer<IndependentPixmap> pixmapBorderExtension = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_TOP_INNER_EXTENSION");
        pixmapBorderExtension->draw(0, 0, width(), height(), &painter);
    }
};

}

LocationsWindow::LocationsWindow(QWidget *parent, Preferences *preferences, gui_locations::LocationsModelManager *locationsModelManager) : QWidget(parent)
  , locationsTabHeightUnscaled_(LOCATIONS_TAB_HEIGHT_INIT)
  , bDragPressed_(false)
  , resizeIconOpacity_(0.5)
{
    setMouseTracking(true);

    connect(&resizeIconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &LocationsWindow::onResizeIconOpacityChanged);

    locationsTab_ = new GuiLocations::LocationsTab(this, preferences, locationsModelManager);
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, qCeil(locationsTabHeightUnscaled_ * G_SCALE));

    connect(locationsTab_, &GuiLocations::LocationsTab::selected, this, &LocationsWindow::selected);
    connect(locationsTab_, &GuiLocations::LocationsTab::clickedOnPremiumStarCity, this, &LocationsWindow::clickedOnPremiumStarCity);
    connect(locationsTab_, &GuiLocations::LocationsTab::addStaticIpClicked, this, &LocationsWindow::addStaticIpClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::clearCustomConfigClicked, this, &LocationsWindow::clearCustomConfigClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::addCustomConfigClicked, this, &LocationsWindow::addCustomConfigClicked);
    connect(locationsTab_, &GuiLocations::LocationsTab::upgradeBannerClicked, this, &LocationsWindow::upgradeBannerClicked);

    connect(preferences, &Preferences::appSkinChanged, this, &LocationsWindow::onAppSkinChanged);

    borderOverlay_ = new BorderOverlay(this);
    borderOverlay_->setGeometry(0, 0, width(), height() - FOOTER_HEIGHT*G_SCALE);
    borderOverlay_->raise();

    setViewportHeight(PersistentState::instance().locationsViewportHeight());
}

int LocationsWindow::tabAndFooterHeight() const
{
    return (locationsTabHeightUnscaled_ + FOOTER_HEIGHT);
}

void LocationsWindow::setViewportHeight(int unscaledHeight, bool dragging)
{
    locationsTabHeightUnscaled_ = unscaledHeight + locationsTab_->headerHeight();
    if (dragging) {
        locationsTab_->updateCurrentWidgetGeometry(unscaledHeight);
    } else {
        locationsTab_->updateLocationWidgetsGeometry(unscaledHeight);
    }
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));
    borderOverlay_->setGeometry(0, 0, width(), height() - FOOTER_HEIGHT*G_SCALE);
    emit heightChanged();
}

int LocationsWindow::getCountVisibleItems()
{
    return locationsTab_->getCountVisibleItems();
}

void LocationsWindow::updateLocationsTabGeometry()
{
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));
    borderOverlay_->setGeometry(0, 0, width(), height() - FOOTER_HEIGHT*G_SCALE);

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
    QRect footerRect(0, height() - FOOTER_HEIGHT * G_SCALE, width(), FOOTER_HEIGHT * G_SCALE);
    CommonGraphics::drawFooter(&p, footerRect);

    // footer handle
    QSharedPointer<IndependentPixmap> footerIcon = ImageResourcesSvg::instance().getIndependentPixmap("FOOTER");
    p.setOpacity(resizeIconOpacity_);
    footerIcon->draw(width() / 2 - footerIcon->width() / 2,
                     height() - FOOTER_HEIGHT*G_SCALE / 2 - footerIcon->height() / 2,
                     &p);
}

void LocationsWindow::resizeEvent(QResizeEvent *event)
{
    locationsTab_->move(0, (event->size().height() - tabAndFooterHeight() * G_SCALE));
    borderOverlay_->setGeometry(0, 0, width(), height() - FOOTER_HEIGHT*G_SCALE);
    QWidget::resizeEvent(event);
}

void LocationsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (bDragPressed_)
    {
        int y_offs = event->globalPosition().toPoint().y() - dragPressPt_.y();
        int unscaled_y_offs = y_offs / G_SCALE;

        int minHeight = MIN_VISIBLE_LOCATIONS * LOCATION_ITEM_HEIGHT;
        int maxHeight = MAX_VISIBLE_LOCATIONS * LOCATION_ITEM_HEIGHT;
        int newHeight = dragInitialViewportHeight_ + unscaled_y_offs;

        newHeight = qMax(minHeight, qMin(maxHeight, newHeight));

        setViewportHeight(newHeight, true);
    }
    else
    {
        QRect middle = getResizeHandleClickableRect();
        if (middle.contains(event->pos()))
        {
            setCursor(Qt::SizeVerCursor);
            startAnAnimation(resizeIconOpacityAnimation_, resizeIconOpacity_, 1.0, ANIMATION_SPEED_FAST);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
            startAnAnimation(resizeIconOpacityAnimation_, resizeIconOpacity_, 0.5, ANIMATION_SPEED_FAST);
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
        dragInitialViewportHeight_ = locationsTab_->unscaledHeightOfItemViewport();

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
        int height = locationsTab_->unscaledHeightOfItemViewport();
        setViewportHeight(height, true); // Make sure to get final size
        setViewportHeight(height, false);
        PersistentState::instance().setLocationsViewportHeight(height);
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

void LocationsWindow::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    locationsTab_->setDataRemaining(bytesUsed, bytesMax);
}

void LocationsWindow::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    locationsTabHeightUnscaled_ = locationsTab_->unscaledHeightOfItemViewport() + locationsTab_->headerHeight();
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH*G_SCALE, qCeil(locationsTabHeightUnscaled_*G_SCALE));
    borderOverlay_->setGeometry(0, 0, width(), height() - FOOTER_HEIGHT*G_SCALE);
    emit heightChanged();
}

void LocationsWindow::onResizeIconOpacityChanged(const QVariant &value)
{
    resizeIconOpacity_ = value.toDouble();
    update();
}
