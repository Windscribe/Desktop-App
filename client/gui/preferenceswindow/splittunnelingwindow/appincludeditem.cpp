#include "appincludeditem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/widgetutils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AppIncludedItem::AppIncludedItem(types::SplitTunnelingApp app, QString iconPath, ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), appIcon_(iconPath), app_(app)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    deleteButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/DELETE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    deleteButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    deleteButton_->setHoverOpacity(OPACITY_FULL);
    connect(deleteButton_, &IconButton::clicked, this, &AppIncludedItem::deleteClicked);

    updatePositions();
}

void AppIncludedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // app icon
    painter->save();
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIconIndependentPixmap(appIcon_);
    if (!p)
    {
        p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    }
    p->draw(PREFERENCES_MARGIN*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE, painter);
    painter->restore();

    // app name
    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);
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

QString AppIncludedItem::getName()
{
    return app_.name;
}

QString AppIncludedItem::getAppIcon()
{
    return appIcon_;
}

void AppIncludedItem::setSelected(bool selected)
{
    selected_ = selected;
    deleteButton_->setSelected(selected);
    update();
}

void AppIncludedItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void AppIncludedItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
}

void AppIncludedItem::setClickable(bool clickable)
{
    deleteButton_->setClickable(clickable);
}

}
