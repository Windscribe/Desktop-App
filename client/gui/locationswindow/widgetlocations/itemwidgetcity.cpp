#include "itemwidgetcity.h"

#include <QPainter>
#include <QCursor>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "tooltips/tooltipcontroller.h"
#include "commongraphics/commongraphics.h"

#include <QDebug>

namespace GuiLocations {

ItemWidgetCity::ItemWidgetCity(IWidgetLocationsInfo *widgetLocationsInfo, const CityModelItem &cityModelItem, QWidget *parent) : IItemWidget(parent)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , cityModelItem_(cityModelItem)
  , showFavIcon_(false)
  , showPingIcon_(false)
  , show10gbpsIcon_(false)
  , showingLatencyAsPingBar_(!widgetLocationsInfo->isShowLatencyInMs())
  , selectable_(false)
  , accented_(false)
  , favorited_(false)
  , pressingFav_(false)
  , curTextOpacity_(OPACITY_HALF)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    cityLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    cityLightWidget_->setText(cityModelItem.city);
    connect(cityLightWidget_.get(), SIGNAL(hoveringChanged(bool)), SLOT(onCityLightWidgetHoveringChanged(bool)));
    nickLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    nickLightWidget_->setText(cityModelItem.nick);
    staticIpLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    staticIpLightWidget_->setText(cityModelItem.staticIp);

    pingTime_ = cityModelItem.pingTimeMs;
    pingIconLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    pingIconLightWidget_->setOpacity(OPACITY_FULL);
    connect(pingIconLightWidget_.get(), SIGNAL(hoveringChanged(bool)), SLOT(onPingIconLightWidgetHoveringChanged(bool)));
    updatePingBarIcon();

    favLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    favLightWidget_->setIcon("locations/FAV_ICON_DESELECTED");
    updateFavoriteIcon();

    tenGbpsLightWidget_ = QSharedPointer<LightWidget>(new LightWidget(this));
    tenGbpsLightWidget_->setIcon("locations/10_GBPS_ICON");
    tenGbpsLightWidget_->setOpacity(OPACITY_FULL);
    update10gbpsIcon();

    textOpacityAnimation_.setDuration(200);
    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityAnimationValueChanged(QVariant)));

    updateScaling();
}

ItemWidgetCity::~ItemWidgetCity()
{
    // qDebug() << "Deleting city: " << name();
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
}

bool ItemWidgetCity::isExpanded() const
{
    return false;
}

const LocationID ItemWidgetCity::getId() const
{
    return cityModelItem_.id;
}

const QString ItemWidgetCity::name() const
{
    return cityModelItem_.city + " " + cityModelItem_.nick;
}

IItemWidget::ItemWidgetType ItemWidgetCity::type()
{
    return ItemWidgetType::CITY;
}

QRect ItemWidgetCity::globalGeometry() const
{
    const QPoint originAsGlobal = mapToGlobal(QPoint(0,0));
    return QRect(originAsGlobal.x(), originAsGlobal.y(), geometry().width(), geometry().height());
}

bool ItemWidgetCity::isForbidden() const
{
    return cityModelItem_.bShowPremiumStarOnly && widgetLocationsInfo_->isFreeSessionStatus();
}

bool ItemWidgetCity::isDisabled() const
{
    return cityModelItem_.isDisabled;
}

bool ItemWidgetCity::isBrokenConfig() const
{
    return cityModelItem_.id.isCustomConfigsLocation() && !cityModelItem_.isCustomConfigCorrect;
}

void ItemWidgetCity::setFavourited(bool favorited)
{
    favorited_ = favorited;
    if (favorited)
    {
        favLightWidget_->setIcon("locations/FAV_ICON_SELECTED");
    }
    else
    {
        favLightWidget_->setIcon("locations/FAV_ICON_DESELECTED");
    }
    update();
}

void ItemWidgetCity::setSelectable(bool selectable)
{
    selectable_ = selectable;
}

void ItemWidgetCity::setAccented(bool accent)
{
    if (selectable_)
    {
        // Do not filter out same-state changes since it is possible to get locked in an updatable state (though relatively rare)
        accented_ = accent;
        if (accent)
        {
            textOpacityAnimation_.setStartValue(curTextOpacity_);
            textOpacityAnimation_.setEndValue(OPACITY_FULL);
            textOpacityAnimation_.start();
            emit accented();
        }
        else
        {
            textOpacityAnimation_.setStartValue(curTextOpacity_);
            textOpacityAnimation_.setEndValue(OPACITY_HALF);
            textOpacityAnimation_.start();
        }
    }
}

void ItemWidgetCity::setAccentedWithoutAnimation(bool accent)
{
    textOpacityAnimation_.stop();
    if (selectable_)
    {
        // Do not filter out same-state changes since it is possible to get locked in an updatable state (though relatively rare)
        accented_ = accent;
        if (accent)
        {
            curTextOpacity_ = OPACITY_FULL;
            emit accented();
        }
        else
        {
            curTextOpacity_ = OPACITY_HALF;
        }
    }
    update();
}

bool ItemWidgetCity::isAccented() const
{
    return accented_;
}

bool ItemWidgetCity::containsCursor() const
{
    return globalGeometry().contains(cursor().pos());
}

bool ItemWidgetCity::containsGlobalPoint(const QPoint &pt)
{
    return globalGeometry().contains(pt);
}

void ItemWidgetCity::setLatencyMs(PingTime pingTime)
{
    pingTime_ = pingTime;
    updatePingBarIcon();
}

void ItemWidgetCity::setShowLatencyMs(bool showLatencyMs)
{
    // time-text drawing handled in paintEvent
    showingLatencyAsPingBar_ = !showLatencyMs;
    updatePingBarIcon();
}

void ItemWidgetCity::updateScaling()
{
    // city text
    cityLightWidget_->setFont(*FontManager::instance().getFont(16, true));
    cityLightWidget_->setRect(QRect(LOCATION_ITEM_MARGIN * G_SCALE * 2 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,
                                     (LOCATION_ITEM_HEIGHT * G_SCALE - cityLightWidget_->textHeight())/2,
                                     cityLightWidget_->truncatedTextWidth(CITY_CAPTION_MAX_WIDTH*G_SCALE),
                                     cityLightWidget_->textHeight()));

    // nick text
    nickLightWidget_->setFont(*FontManager::instance().getFont(16, false));
    nickLightWidget_->setRect(QRect(cityLightWidget_->rect().x() + cityLightWidget_->rect().width() + 8*G_SCALE,
                                     (LOCATION_ITEM_HEIGHT * G_SCALE - nickLightWidget_->textHeight())/2,
                                     nickLightWidget_->truncatedTextWidth(CITY_CAPTION_MAX_WIDTH*G_SCALE),
                                     nickLightWidget_->textHeight()));

    // static ip text
    staticIpLightWidget_->setFont(*FontManager::instance().getFont(13, false));
    staticIpLightWidget_->setRect(QRect((WINDOW_WIDTH*G_SCALE - staticIpLightWidget_->truncatedTextWidth(CITY_CAPTION_MAX_WIDTH*G_SCALE) - 44*G_SCALE),
                                         (LOCATION_ITEM_HEIGHT * G_SCALE - staticIpLightWidget_->textHeight())/2,
                                         staticIpLightWidget_->truncatedTextWidth(CITY_CAPTION_MAX_WIDTH*G_SCALE),
                                         staticIpLightWidget_->textHeight()));

    recreateTextLayouts();

    // ping icon
    int scaledX = (WINDOW_WIDTH - 16 - LOCATION_ITEM_MARGIN) * G_SCALE;
    int scaledY = (LOCATION_ITEM_HEIGHT - 16) / 2 * G_SCALE - 1*G_SCALE;
    pingIconLightWidget_->setRect(QRect(scaledX, scaledY, 16 * G_SCALE, 16 * G_SCALE));

    // fav icon
    int favoriteHeight = 14 * G_SCALE;
    favLightWidget_->setRect(QRect(24*G_SCALE,
                                     (LOCATION_ITEM_HEIGHT*G_SCALE - favoriteHeight)/2,
                                     16 * G_SCALE,
                                     favoriteHeight));

    // 10gbps badge icon
    int badgeHeight = 14 * G_SCALE;
    tenGbpsLightWidget_->setRect(QRect(scaledX - (32 * G_SCALE),
                                     (LOCATION_ITEM_HEIGHT*G_SCALE - badgeHeight)/2,
                                     16 * G_SCALE,
                                     badgeHeight));
    update();

}

void ItemWidgetCity::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // background
    QPainter painter(this);
    double initOpacity = painter.opacity();
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE),
                      FontManager::instance().getMidnightColor());

    if (cityModelItem_.id.isStaticIpsLocation())
    {
        // static ip icon
        QSharedPointer<IndependentPixmap> datacenterPixmap;
        if (cityModelItem_.staticIpType.compare("dc", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DATACENTER_ICON");
        }
        else if (cityModelItem_.staticIpType.compare("res", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/RES_IP_ICON");
        }
        else
        {
            Q_ASSERT(false);
        }

        if (datacenterPixmap)
        {
            datacenterPixmap->draw(LOCATION_ITEM_MARGIN_TO_LINE * G_SCALE,
                                   (LOCATION_ITEM_HEIGHT*G_SCALE - datacenterPixmap->height()) / 2, &painter);
        }

        // static ip text
        // qDebug() << "Drawing static location";
        painter.setOpacity(0.5);
        painter.setPen(Qt::white);
        staticIpTextLayout_->draw(&painter, QPoint(staticIpLightWidget_->rect().x(), staticIpLightWidget_->rect().y()));

    }
    else if (isForbidden())
    {
        // pro star
        QSharedPointer<IndependentPixmap> premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        premiumStarPixmap->draw(LOCATION_ITEM_MARGIN * G_SCALE,
                                (LOCATION_ITEM_HEIGHT*G_SCALE - premiumStarPixmap->height()) / 2, &painter);
    }
    else if (cityModelItem_.id.isCustomConfigsLocation())
    {
        QString type = cityModelItem_.customConfigType;
        if (!type.isEmpty())
        {
            QSharedPointer<IndependentPixmap> configPixmap = ImageResourcesSvg::instance().getIndependentPixmap(
                QString("locations/%1_ICON").arg(type.toUpper()));
            if (configPixmap) {
                painter.setOpacity(curTextOpacity_);
                configPixmap->draw(LOCATION_ITEM_MARGIN * G_SCALE,
                                   (LOCATION_ITEM_HEIGHT*G_SCALE - configPixmap->height()) / 2, &painter);
            }
        }
    }
    else // fav
    {
        QSharedPointer<IndependentPixmap> favIcon = ImageResourcesSvg::instance().getIndependentPixmap(favLightWidget_->icon());
        if (favIcon)
        {
            painter.setOpacity(favLightWidget_->opacity());
            favIcon->draw(favLightWidget_->rect().x(), favLightWidget_->rect().y(), &painter);
        }
    }

    // text
    painter.save();
    painter.setOpacity(curTextOpacity_);
    painter.setPen(Qt::white);
    cityTextLayout_->draw(&painter, QPoint(cityLightWidget_->rect().x(), cityLightWidget_->rect().y()));

    // city text for non-static and non-custom views only
    if (!cityModelItem_.id.isStaticIpsLocation() && !cityModelItem_.id.isCustomConfigsLocation())
    {
        nickTextLayout_->draw(&painter, QPoint(nickLightWidget_->rect().x(), nickLightWidget_->rect().y()));
    }
    painter.restore();


    // only show disabled locations to pro users
    bool disabled = isForbidden() ? false : cityModelItem_.isDisabled;

    if (disabled)
    {
        QSharedPointer<IndependentPixmap> consIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
        if (consIcon)
        {
            painter.setOpacity(OPACITY_HALF);
            consIcon->draw((WINDOW_WIDTH - LOCATION_ITEM_MARGIN)*G_SCALE - consIcon->width(),
                       (LOCATION_ITEM_HEIGHT*G_SCALE - consIcon->height()) / 2,
                       &painter);
        }
    }
    else if (isBrokenConfig())
    {
        painter.save();
        QSharedPointer<IndependentPixmap> pingIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/ERROR_ICON");
        pingIcon->draw(pingIconLightWidget_->rect().x(), pingIconLightWidget_->rect().y(), &painter);
        painter.restore();
    }
    else if (!showingLatencyAsPingBar_)
    {
        // draw bubble
        int latencyRectWidth = 33;
        int latencyRectHeight = 20;
        int scaledX = (WINDOW_WIDTH - latencyRectWidth - 8) * G_SCALE;
        int scaledY = (LOCATION_ITEM_HEIGHT - latencyRectHeight) / 2 *G_SCALE - 1*G_SCALE;
        QRect latencyRect(scaledX, scaledY,
                          latencyRectWidth * G_SCALE, latencyRectHeight *G_SCALE);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(QColor(40, 45, 61)));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(latencyRect, 4*G_SCALE, 4*G_SCALE);

        // draw latency text
        QFont font = *FontManager::instance().getFont(11, false);
        QString latencyText = QString::number(pingTime_.toInt());
        painter.setBrush(Qt::white);
        painter.setPen(Qt::white);
        painter.setFont(font);
        painter.drawText(QRect((scaledX + latencyRectWidth/2*G_SCALE) - CommonGraphics::textWidth(latencyText, font)/2,
                               (scaledY + latencyRectHeight/2*G_SCALE) - CommonGraphics::textHeight(font)/2 ,
                               CommonGraphics::textWidth(latencyText, font),
                               CommonGraphics::textHeight(font)),
                         latencyText);
    }
    else if (showPingIcon_)
    {
        QSharedPointer<IndependentPixmap> pingIcon = ImageResourcesSvg::instance().getIndependentPixmap(pingIconLightWidget_->icon());
        if (pingIcon)
        {
            painter.save();
            painter.setOpacity(pingIconLightWidget_->opacity());
            pingIcon->draw(pingIconLightWidget_->rect().x(), pingIconLightWidget_->rect().y(), &painter);
            painter.restore();
        }
    }

    if (show10gbpsIcon_)
    {
        QSharedPointer<IndependentPixmap> theIcon = ImageResourcesSvg::instance().getIndependentPixmap(tenGbpsLightWidget_->icon());
        if (theIcon)
        {
            painter.save();
            painter.setOpacity(tenGbpsLightWidget_->opacity());
            theIcon->draw(tenGbpsLightWidget_->rect().x(), tenGbpsLightWidget_->rect().y(), &painter);
            painter.restore();
        }
    }

    // background line
	// TODO: lines drawn like this do not scale -- fix
    int left = static_cast<int>(24 * G_SCALE);
    int right = static_cast<int>(WINDOW_WIDTH * G_SCALE - 8*G_SCALE);
    int bottom = static_cast<int>(LOCATION_ITEM_HEIGHT*G_SCALE) -1; // 1 is not scaled since we want bottom-most pixel in geometry
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
    painter.setOpacity(initOpacity);
    painter.setPen(pen);
    painter.drawLine(left, bottom - 1, right, bottom - 1);
    painter.drawLine(left, bottom, right, bottom);

    if (widgetLocationsInfo_->isShowLocationLoad() && cityModelItem_.locationLoad > 0)
    {
        Qt::GlobalColor penColor;
        if (cityModelItem_.locationLoad < 60) {
            penColor = Qt::green;
        }
        else if (cityModelItem_.locationLoad < 90) {
            penColor = Qt::yellow;
        }
        else {
            penColor = Qt::red;
        }
        int rightX = left + ((right - left) * cityModelItem_.locationLoad / 100);
        QPen penLoad(penColor);
        penLoad.setWidth(1);
        painter.setOpacity(curTextOpacity_);
        painter.setPen(penLoad);
        painter.drawLine(left, bottom - 1, rightX, bottom - 1);
        painter.drawLine(left, bottom, rightX, bottom);
        painter.setOpacity(1.0);
    }
}

void ItemWidgetCity::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    emit hoverEnter();
}

void ItemWidgetCity::leaveEvent(QEvent *event)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
    QWidget::leaveEvent(event);
}

void ItemWidgetCity::mouseMoveEvent(QMouseEvent *event)
{
    if (cityModelItem_.id.isCustomConfigsLocation())
    {
        if (cityLightWidget_->rect().contains(event->pos()))
        {
            cityLightWidget_->setHovering(true);
        }
        else
        {
            cityLightWidget_->setHovering(false);
        }
    }
    else
    {
        cityLightWidget_->setHovering(false);
    }

    if (showingLatencyAsPingBar_ || isBrokenConfig())
    {
        if (pingIconLightWidget_->rect().contains(event->pos()))
        {
            pingIconLightWidget_->setHovering(true);
        }
        else
        {
            pingIconLightWidget_->setHovering(false);
        }
    }
    else
    {
        pingIconLightWidget_->setHovering(false);
    }
}

void ItemWidgetCity::mousePressEvent(QMouseEvent *event)
{
    if (!cityModelItem_.id.isStaticIpsLocation() && !cityModelItem_.id.isCustomConfigsLocation())
    {
        pressingFav_ = false;
        if (favLightWidget_->rect().contains(event->pos()))
        {
            pressingFav_ = true;
        }
    }

    QAbstractButton::mousePressEvent(event);
}

void ItemWidgetCity::mouseReleaseEvent(QMouseEvent *event)
{
    if (!cityModelItem_.id.isStaticIpsLocation() && !cityModelItem_.id.isCustomConfigsLocation())
    {
        if (pressingFav_)
        {
            pressingFav_ = false;
            if (favLightWidget_->rect().contains(event->pos()))
            {
                clickFavorite();
            }
            return; // prevent city selection when click started on fav
        }
        else if (favLightWidget_->rect().contains(event->pos()))
        {
            return; // prevent city selection when click ends on fav
        }
    }

    QAbstractButton::mouseReleaseEvent(event);
}

void ItemWidgetCity::onPingIconLightWidgetHoveringChanged(bool hovering)
{
    if (hovering)
    {
        if (isBrokenConfig())
        {
            QString text = cityModelItem_.customConfigErrorMessage;
            if (text.isEmpty()) text = tr("Unknown Config Error");

            QPoint pt = mapToGlobal(QPoint(pingIconLightWidget_->rect().center().x(), pingIconLightWidget_->rect().top() - 3*G_SCALE));
            TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
            ti.x = pt.x();
            ti.y = pt.y();
            ti.title = text;
            ti.tailtype = TOOLTIP_TAIL_BOTTOM;
            ti.tailPosPercent = 0.8;
            TooltipController::instance().showTooltipBasic(ti);
        }
        else if (showingLatencyAsPingBar_)
        {
            QPoint pt = mapToGlobal(QPoint(pingIconLightWidget_->rect().center().x(), pingIconLightWidget_->rect().top() - 3*G_SCALE));
            TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_PING_TIME);
            ti.x = pt.x();
            ti.y = pt.y();
            ti.title = QString("%1 Ms").arg(pingTime_.toInt());
            ti.tailtype = TOOLTIP_TAIL_BOTTOM;
            ti.tailPosPercent = 0.5;
            TooltipController::instance().showTooltipBasic(ti);
        }
    }
    else
    {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
    }
}

void ItemWidgetCity::onCityLightWidgetHoveringChanged(bool hovering)
{
    if (hovering)
    {
        QString text = CommonGraphics::maybeTruncatedText(cityLightWidget_->text(),
                                                          cityLightWidget_->font(),
                                                          static_cast<int>(CITY_CAPTION_MAX_WIDTH*G_SCALE));
        if (text != cityLightWidget_->text())
        {
            QPoint pt = mapToGlobal(QPoint(cityLightWidget_->rect().x() + cityLightWidget_->rect().width()*0.1,
                                           cityLightWidget_->rect().top() - 3*G_SCALE));
            TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
            ti.x = pt.x();
            ti.y = pt.y();
            ti.title = cityLightWidget_->text();
            ti.tailtype = TOOLTIP_TAIL_BOTTOM;
            ti.tailPosPercent = 0.1;
            TooltipController::instance().showTooltipBasic(ti);
        }
    }
    else
    {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    }
}

void ItemWidgetCity::onTextOpacityAnimationValueChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void ItemWidgetCity::updateFavoriteIcon()
{
    if (isForbidden())
    {
        showFavIcon_ = false;
    }
    else
    {
        if (cityModelItem_.isFavorite)
        {
            favLightWidget_->setIcon("locations/FAV_ICON_SELECTED");
            favorited_ = true;
        }
        showFavIcon_ = true;
    }
    update();
}

void ItemWidgetCity::clickFavorite()
{
    emit favoriteClicked(this, !favorited_);
}

void ItemWidgetCity::updatePingBarIcon()
{
    pingIconLightWidget_->setIcon(pingIconNameString(pingTime_.toConnectionSpeed()));

    bool disabled = isForbidden() ? false : cityModelItem_.isDisabled;

    if (disabled) // showing construction icon for disabled pro
    {
        showPingIcon_ = false;
    }
    else if (showingLatencyAsPingBar_) // show ping bar
    {
        showPingIcon_ = true;
    }
    else // show ms ping
    {
        showPingIcon_ = false;
    }

    update();
}

const QString ItemWidgetCity::pingIconNameString(int connectionSpeedIndex)
{
    if (connectionSpeedIndex == 0)
    {
        return "locations/LOCATION_PING_BARS0";
    }
    else if (connectionSpeedIndex == 1)
    {
        return "locations/LOCATION_PING_BARS1";
    }
    else if (connectionSpeedIndex == 2)
    {
        return "locations/LOCATION_PING_BARS2";
    }
    else
    {
        return "locations/LOCATION_PING_BARS3";
    }
}

void ItemWidgetCity::recreateTextLayouts()
{
    // shortening should only be required for configs
    QString cityText = CommonGraphics::maybeTruncatedText(cityLightWidget_->text(),
                                                          cityLightWidget_->font(),
                                                          static_cast<int>(CITY_CAPTION_MAX_WIDTH * G_SCALE));

    cityTextLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(cityText, cityLightWidget_->font()));
    cityTextLayout_->beginLayout();
    cityTextLayout_->createLine();
    cityTextLayout_->endLayout();
    cityTextLayout_->setCacheEnabled(true);

    nickTextLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(nickLightWidget_->text(), nickLightWidget_->font()));
    nickTextLayout_->beginLayout();
    nickTextLayout_->createLine();
    nickTextLayout_->endLayout();
    nickTextLayout_->setCacheEnabled(true);

    staticIpTextLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(staticIpLightWidget_->text(),staticIpLightWidget_->font()));
    staticIpTextLayout_->beginLayout();
    staticIpTextLayout_->createLine();
    staticIpTextLayout_->endLayout();
    staticIpTextLayout_->setCacheEnabled(true);
}

void ItemWidgetCity::update10gbpsIcon()
{
    show10gbpsIcon_ = (cityModelItem_.linkSpeed == 10000);
    update();
}

}
