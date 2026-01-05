#include "splittunnelingappsitem.h"

#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

SplitTunnelingAppsItem::SplitTunnelingAppsItem(ScalableGraphicsObject *parent)
  : CommonGraphics::BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)
{
    searchButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/MAGNIFYING_GLASS", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(searchButton_, &IconButton::clicked, this, &SplitTunnelingAppsItem::searchClicked);

    addButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "locations/EXPAND_ICON", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(addButton_, &IconButton::clicked, this, &SplitTunnelingAppsItem::addClicked);
}

void SplitTunnelingAppsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    QString text(tr("Search/Add Apps"));
    QString elidedText = text;

    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    painter->setOpacity(OPACITY_HALF);
    painter->setPen(Qt::white);

    int availableWidth = searchButton_->pos().x() - PREFERENCES_MARGIN_X*G_SCALE;

    QFontMetrics fm(font);
    if (availableWidth < fm.horizontalAdvance(text)) {
        elidedText = fm.elidedText(text, Qt::ElideRight, availableWidth, 0);
    }

    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, 0, 0), Qt::AlignVCenter, elidedText);
}

void SplitTunnelingAppsItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void SplitTunnelingAppsItem::updatePositions()
{
    searchButton_->setPos(boundingRect().width() - 2*ICON_WIDTH*G_SCALE - 2*PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2);
    addButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2);
}

void SplitTunnelingAppsItem::setClickable(bool clickable)
{
    searchButton_->setClickable(clickable);
    addButton_->setClickable(clickable);
}

} // namespace PreferencesWindow
