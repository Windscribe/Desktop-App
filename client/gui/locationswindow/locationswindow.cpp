#include "locationswindow.h"

#include <QPainter>
#include <QtMath>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

#include <QDebug>

LocationsWindow::LocationsWindow(QWidget *parent, LocationsModel *locationsModel) : QWidget(parent)
  , locationsTabHeightUnscaled_(LOCATIONS_TAB_HEIGHT_INIT)
  , bDragPressed_(false)
{
    setMouseTracking(true);

    locationsTab_ = new GuiLocations::LocationsTab(this, locationsModel);
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, qCeil(locationsTabHeightUnscaled_ * G_SCALE));

    footerTopStrip_ = new GuiLocations::FooterTopStrip(this);
    updateFooterOverlayGeo();

    connect(locationsTab_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(locationsTab_, SIGNAL(clickedOnPremiumStarCity()), SIGNAL(clickedOnPremiumStarCity()));
    connect(locationsTab_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(locationsTab_, SIGNAL(addStaticIpClicked()), SIGNAL(addStaticIpClicked()));
    connect(locationsTab_, SIGNAL(clearCustomConfigClicked()), SIGNAL(clearCustomConfigClicked()));
    connect(locationsTab_, SIGNAL(addCustomConfigClicked()), SIGNAL(addCustomConfigClicked()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
}

int LocationsWindow::tabAndFooterHeight() const
{
    return (locationsTabHeightUnscaled_ + FOOTER_HEIGHT);
}

void LocationsWindow::setCountVisibleItemSlots(int cnt)
{
    locationsTab_->setCountVisibleItemSlots(cnt);
    // Previously there were issues directly grabbing locationsTab height... keeping a cache somehow helped. Not sure if the original issue persists
    locationsTabHeightUnscaled_ = locationsTab_->unscaledHeightOfItemViewport() + GuiLocations::LocationsTab::TAB_HEADER_HEIGHT;
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, qCeil(locationsTabHeightUnscaled_ * G_SCALE));
    updateFooterOverlayGeo();

    emit heightChanged();
}

int LocationsWindow::getCountVisibleItems()
{
    return locationsTab_->getCountVisibleItems();
}

void LocationsWindow::setOnlyConfigTabVisible(bool onlyConfig)
{
    locationsTab_->setOnlyConfigTabVisible(onlyConfig);
}

void LocationsWindow::handleKeyReleaseEvent(QKeyEvent *event)
{
    // qDebug() << "LocationsWindow::handleKeyReleaseEvent";
    locationsTab_->handleKeyReleaseEvent(event);
}

void LocationsWindow::handleKeyPressEvent(QKeyEvent *event)
{
    locationsTab_->handleKeyPressEvent(event);
}

void LocationsWindow::updateLocationsTabGeometry()
{
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, qCeil(locationsTabHeightUnscaled_ * G_SCALE));
    updateFooterOverlayGeo();

    locationsTab_->updateLocationWidgetsGeometry(locationsTab_->unscaledHeightOfItemViewport());
    locationsTab_->updateIconRectsAndLine();
    locationsTab_->update();
}

void LocationsWindow::updateScaling()
{
    locationsTab_->updateScaling();
    updateFooterOverlayGeo();
}

void LocationsWindow::setMuteAccentChanges(bool mute)
{
    locationsTab_->setMuteAccentChanges(mute);
}

void LocationsWindow::hideSearchTabWithoutAnimation()
{
    locationsTab_->hideSearchTabWithoutAnimation();
}

GuiLocations::LocationsTab::LocationTabEnum LocationsWindow::currentTab()
{
    return locationsTab_->currentTab();
}

void LocationsWindow::setLatencyDisplay(ProtoTypes::LatencyDisplayType l)
{
    locationsTab_->setLatencyDisplay(l);
}

void LocationsWindow::setShowLocationLoad(bool showLocationLoad)
{
    locationsTab_->setShowLocationLoad(showLocationLoad);
}

void LocationsWindow::setCustomConfigsPath(QString path)
{
    locationsTab_->setCustomConfigsPath(path);
}

void LocationsWindow::onLanguageChanged()
{
    locationsTab_->updateLanguage();
}

void LocationsWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // qDebug() << "LocationsWindow::paintEvent - geo: " << geometry();

    // footer background
    QColor footerColor = FontManager::instance().getLocationsFooterColor();
#ifdef Q_OS_MAC
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
        int y_offs = event->globalPosition().y() - dragPressPt_.y();

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

void LocationsWindow::updateFooterOverlayGeo()
{
    int bringItBackAbit = qCeil(GuiLocations::FooterTopStrip::HEIGHT * G_SCALE);
    int tabHeight = locationsTab_->geometry().height();
    int newFooterPos = tabHeight - bringItBackAbit;

    footerTopStrip_->setGeometry(0,
                                 newFooterPos,
                                 WINDOW_WIDTH*G_SCALE,
                                 bringItBackAbit);
    footerTopStrip_->update();
}
