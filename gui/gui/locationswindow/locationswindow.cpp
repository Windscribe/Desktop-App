#include "locationswindow.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

#include <QDebug>

LocationsWindow::LocationsWindow(QWidget *parent, LocationsModel *locationsModel) : QWidget(parent),
    locationsTabHeight_(LOCATIONS_TAB_HEIGHT), bDragPressed_(false)
{
    setMouseTracking(true);

    locationsTab_ = new GuiLocations::LocationsTab(this, locationsModel);
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, locationsTabHeight_ * G_SCALE);

    connect(locationsTab_, SIGNAL(selected(LocationID)), SIGNAL(selected(LocationID)));
    connect(locationsTab_, SIGNAL(clickedOnPremiumStarCity()), SIGNAL(clickedOnPremiumStarCity()));
    connect(locationsTab_, SIGNAL(switchFavorite(LocationID,bool)), SIGNAL(switchFavorite(LocationID,bool)));
    connect(locationsTab_, SIGNAL(addStaticIpClicked()), SIGNAL(addStaticIpClicked()));
    connect(locationsTab_, SIGNAL(clearCustomConfigClicked()), SIGNAL(clearCustomConfigClicked()));
    connect(locationsTab_, SIGNAL(addCustomConfigClicked()), SIGNAL(addCustomConfigClicked()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    footerColor_ = QColor(26, 39, 58);
}

int LocationsWindow::tabAndFooterHeight() const
{
    return (locationsTabHeight_ + BOTTOM_AREA_HEIGHT);
}

void LocationsWindow::setCountVisibleItemSlots(int cnt)
{
    locationsTab_->setCountVisibleItemSlots(cnt);
    locationsTabHeight_ = locationsTab_->unscaledHeight() + 48 ; // hide last location separator line // TODO: issue here?
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, locationsTabHeight_ * G_SCALE);
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

void LocationsWindow::updateLocationsTabGeometry()
{
    locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, locationsTabHeight_ * G_SCALE);

    locationsTab_->updateLocationWidgetsGeometry(locationsTab_->unscaledHeight());
    locationsTab_->updateIconRectsAndLine();
    locationsTab_->update();
}

void LocationsWindow::updateScaling()
{
    locationsTab_->updateScaling();
}

void LocationsWindow::setLatencyDisplay(ProtoTypes::LatencyDisplayType l)
{
    locationsTab_->setLatencyDisplay(l);
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
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // qDebug() << "LocatiosWindow::paintEvent - geo: " << geometry();

    // draw footer background
    int notchAdjustment = 2;
#ifdef Q_OS_MAC
        p.setPen(footerColor_);
        p.setBrush(footerColor_);
        p.drawRoundedRect(QRect(0, height() - BOTTOM_AREA_HEIGHT * G_SCALE, width(), BOTTOM_AREA_HEIGHT * G_SCALE), 8, 8);
        p.fillRect(QRect(0, height() - BOTTOM_AREA_HEIGHT * G_SCALE, width(), (BOTTOM_AREA_HEIGHT - 2) * G_SCALE / 2), QBrush(footerColor_));
#else
        p.fillRect(QRect(0, height() - BOTTOM_AREA_HEIGHT * G_SCALE, width(), (BOTTOM_AREA_HEIGHT - 2) * G_SCALE), QBrush(footerColor_));
#endif

    // draw handle
    QRect middle(width() / 2 - 12* G_SCALE, height() - BOTTOM_AREA_HEIGHT * G_SCALE / 2 - notchAdjustment * G_SCALE, 24 * G_SCALE, 3 * G_SCALE);
    p.setOpacity(0.5);
    p.fillRect(QRect(middle.left(), middle.top(), middle.width(), 2 * G_SCALE), QBrush(Qt::white));
}

void LocationsWindow::resizeEvent(QResizeEvent *event)
{
    locationsTab_->move(0, (event->size().height() - tabAndFooterHeight() * G_SCALE));
    QWidget::resizeEvent(event);
    //locationsTab_->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, locationsTabHeight_ * G_SCALE);
}

void LocationsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (bDragPressed_)
    {
        int y_offs = event->globalPos().y() - dragPressPt_.y();

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
        QRect middle = getResizeMiddleRect();
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
    QRect middle = getResizeMiddleRect();
    if (middle.contains(event->pos()))
    {
        bDragPressed_ = true;
        dragPressPt_ = event->globalPos();
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

QRect LocationsWindow::getResizeMiddleRect()
{
    return QRect(width() / 2 - 14*G_SCALE,
                 height() - BOTTOM_AREA_HEIGHT*G_SCALE / 2 - 4*G_SCALE,
                 28*G_SCALE,
                 8*G_SCALE);
}
