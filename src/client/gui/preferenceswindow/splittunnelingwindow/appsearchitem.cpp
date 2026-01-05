#include "appsearchitem.h"

#include <QIcon>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AppSearchItem::AppSearchItem(types::SplitTunnelingApp app, ScalableGraphicsObject *parent)
  : BaseItem (parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), opacity_(OPACITY_HALF), app_(app)
{
    if (app_.icon.isEmpty()) {
        app_.icon = "preferences/WHITE_QUESTION_MARK_ICON";
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
#if !defined (Q_OS_LINUX)
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIconIndependentPixmap(app_.icon);
    if (!p) {
        p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    }
    p->draw(PREFERENCES_MARGIN_X*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE, painter);
#else
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    QPixmap pixmap = QIcon::fromTheme(app_.icon, p->getIcon()).pixmap(QSize(APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE));

    painter->drawPixmap(PREFERENCES_MARGIN_X*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, pixmap);
#endif

    // add icon
    painter->setPen(Qt::white);
    painter->setOpacity(opacity_);
    QSharedPointer<IndependentPixmap> addIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");
    addIcon->draw(boundingRect().width() - (PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2, painter);

    // text
    painter->setPen(Qt::white);
    painter->setOpacity(opacity_);

    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    QFontMetrics fm(font);
    QString elidedName = fm.elidedText(app_.name,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (3*PREFERENCES_MARGIN_X + APP_ICON_MARGIN_X + APP_ICON_WIDTH + ICON_WIDTH)*G_SCALE);
    painter->drawText(boundingRect().adjusted((PREFERENCES_MARGIN_X + APP_ICON_WIDTH + APP_ICON_MARGIN_X)*G_SCALE,
                                              0,
                                              -(2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE,
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
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
    return app_.icon;
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
    app_.icon = icon;
    update();
}

}
