#include "appsearchitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/widgetutils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AppSearchItem::AppSearchItem(types::SplitTunnelingApp app, QString appIconPath, ScalableGraphicsObject *parent)
  : BaseItem (parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), opacity_(OPACITY_HALF), app_(app), appIcon_(appIconPath)
{
    if (appIcon_ == "")
    {
        appIcon_ = "preferences/WHITE_QUESTION_MARK_ICON";
    }

    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &AppSearchItem::onOpacityChanged);
    connect(this, &BaseItem::hoverEnter, this, &AppSearchItem::onHoverEnter);
    connect(this, &BaseItem::hoverLeave, this, &AppSearchItem::onHoverLeave);
}

void AppSearchItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // app icon
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIconIndependentPixmap(appIcon_);
    if (!p)
    {
        p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    }
    p->draw(PREFERENCES_MARGIN*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE, painter);

    // add icon
    painter->setPen(Qt::white);
    painter->setOpacity(opacity_);
    QSharedPointer<IndependentPixmap> addIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");
    addIcon->draw(boundingRect().width() - (PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE, PREFERENCES_MARGIN*G_SCALE, painter);

    // text
    painter->setPen(Qt::white);
    painter->setOpacity(opacity_);

    QFont *font = FontManager::instance().getFont(12, false);
    painter->setFont(*font);
    QFontMetrics fm(*font);
    QString elidedName = fm.elidedText(app_.name,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (3*PREFERENCES_MARGIN + APP_ICON_MARGIN_X + APP_ICON_WIDTH + ICON_WIDTH)*G_SCALE);
    painter->drawText(boundingRect().adjusted((PREFERENCES_MARGIN + APP_ICON_WIDTH + APP_ICON_MARGIN_X)*G_SCALE,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      elidedName);
}

QString AppSearchItem::getName()
{
    return app_.name;
}

QString AppSearchItem::getFullName()
{
    return app_.fullName;
}

QString AppSearchItem::getAppIcon()
{
    return appIcon_;
}

void AppSearchItem::setSelected(bool selected)
{
    selected_ = selected;

    double targetOpacity = OPACITY_FULL;
    if (!selected)
    {
        targetOpacity = OPACITY_HALF;
    }
    startAnAnimation<double>(opacityAnimation_, opacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void AppSearchItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
}

void AppSearchItem::onOpacityChanged(const QVariant &value)
{
    opacity_ = value.toDouble();
    update();
}

void AppSearchItem::onHoverEnter()
{
    startAnAnimation<double>(opacityAnimation_, opacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void AppSearchItem::onHoverLeave()
{
    startAnAnimation<double>(opacityAnimation_, opacity_, OPACITY_HALF, ANIMATION_SPEED_FAST);
}

void AppSearchItem::setAppIcon(QString icon)
{
    appIcon_ = icon;
    update();
}

}
