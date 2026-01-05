#include "preferencegroup.h"

#include <QDesktopServices>
#include <QPainter>
#include <QUrl>
#include "commongraphics/commongraphics.h"
#include "commongraphics/dividerline.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

#define INTERNAL_INDEX(x) (x*2)

PreferenceGroup::PreferenceGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
    : BaseItem(parent, 0), desc_(desc), errorDesc_(""), descUrl_(descUrl), descRightMargin_(PREFERENCES_MARGIN_X), descHeight_(0),
      borderWidth_(2), icon_(new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/INFO_ICON", "", this, OPACITY_SIXTY)),
      error_(false), drawBackground_(true), descHidden_(false)
{
    connect(icon_, &IconButton::clicked, this, &PreferenceGroup::onIconClicked);

    updatePositions();
}

void PreferenceGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    // border
    QPainterPath path;
    path.addRoundedRect(boundingRect().adjusted(borderWidth_*G_SCALE-1,
                                                borderWidth_*G_SCALE-1,
                                                -borderWidth_*G_SCALE+1,
                                                -borderWidth_*G_SCALE+1),
                        (ROUNDED_RECT_RADIUS+borderWidth_-1)*G_SCALE, (ROUNDED_RECT_RADIUS+borderWidth_-1)*G_SCALE);
    QPen pen = painter->pen();
    pen.setWidth(borderWidth_*G_SCALE);
    QPainterPathStroker stroker(pen);
    painter->setPen(Qt::NoPen);
    painter->fillPath(stroker.createStroke(path), QColor(255, 255, 255, 26));
    painter->setPen(Qt::SolidLine);

    // background
    if (size() != 0 && drawBackground_) {
        QPainterPath path;
        painter->setOpacity(0.08);
        path.addRoundedRect(boundingRect().adjusted(borderWidth_*G_SCALE,
                                                    borderWidth_*G_SCALE,
                                                    -borderWidth_*G_SCALE,
                                                    -borderWidth_*G_SCALE),
                            ROUNDED_RECT_RADIUS*G_SCALE, ROUNDED_RECT_RADIUS*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, Qt::white);
        painter->setPen(Qt::SolidLine);
    }


    // description text
    if (descHidden_) {
        return;
    }

    int descVMargin = (DESCRIPTION_MARGIN + borderWidth_)*G_SCALE;
    if (size() == 0) {
        // use full margin if there are no items in this group
        descVMargin = (PREFERENCES_MARGIN_Y + borderWidth_)*G_SCALE;
    }

    if (error_) {
        painter->setOpacity(OPACITY_FULL);
        painter->setPen(Qt::red);
        QSharedPointer<IndependentPixmap> errorIcon_ = ImageResourcesSvg::instance().getIndependentPixmap("preferences/ERROR");
        errorIcon_->draw(PREFERENCES_MARGIN_X*G_SCALE,
                         boundingRect().height() - descHeight_/2 - descVMargin - ICON_HEIGHT/2*G_SCALE,
                         ICON_WIDTH*G_SCALE,
                         ICON_HEIGHT*G_SCALE,
                         painter);
        QFont font = FontManager::instance().getFont(12,  QFont::Normal);
        painter->setFont(font);
        painter->drawText(boundingRect().adjusted((2*PREFERENCES_MARGIN_X+ICON_WIDTH)*G_SCALE,
                                                  boundingRect().height() - descVMargin - descHeight_,
                                                  -descRightMargin_,
                                                  -(DESCRIPTION_MARGIN + borderWidth_)*G_SCALE),
                          errorDesc_);
    } else if (!desc_.isEmpty()) {
        painter->setPen(Qt::white);
        painter->setOpacity(OPACITY_SEVENTY);
        QFont font = FontManager::instance().getFont(12,  QFont::Normal);
        painter->setFont(font);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  boundingRect().height() - descVMargin - descHeight_,
                                                  -descRightMargin_,
                                                  -(DESCRIPTION_MARGIN + borderWidth_)*G_SCALE),
                          desc_);
    }
}

void PreferenceGroup::addItem(CommonGraphics::BaseItem *item, bool isWideDividerLine)
{
    itemsExternal_ << item;

    if (!items_.empty()) {
        CommonGraphics::DividerLine *line;
        if (!isWideDividerLine) {
            line = new CommonGraphics::DividerLine(this);
        } else {
            line = new CommonGraphics::DividerLine(this, PAGE_WIDTH, 0);
        }

        connect(line, &BaseItem::heightChanged, this, &PreferenceGroup::updatePositions);
        connect(line, &BaseItem::hidden, this, &PreferenceGroup::onHidden);
        if (firstVisibleItem() < 0) {
            // no visible items, don't show line
            line->hide(false);
        }
        items_ << line;
    }
    connect(item, &BaseItem::heightChanged, this, &PreferenceGroup::updatePositions);
    connect(item, &BaseItem::hidden, this, &PreferenceGroup::onHidden);
    items_ << item;

    updatePositions();
    emit itemsChanged();
}

void PreferenceGroup::clearItems(bool skipFirst)
{
    int sz = skipFirst ? 1 : 0;

    toDelete_.clear();

    while(itemsExternal_.size() > sz) {
        itemsExternal_.removeAt(sz);
    }

    while(items_.size() > sz) {
        items_[sz]->disconnect();
        items_[sz]->deleteLater();
        items_.removeAt(sz);
    }
    updatePositions();
    emit itemsChanged();
}

void PreferenceGroup::onHidden()
{
    BaseItem *item = static_cast<BaseItem *>(sender());

    if (toDelete_.removeOne(item)) {
        item->disconnect();
        items_.removeOne(item);
        itemsExternal_.removeOne(item);
        item->deleteLater();
        emit itemsChanged();
    }

    int firstVisible = firstVisibleItem();
    if (firstVisible > 0 && isDividerLine(firstVisible)) {
        items_[firstVisible]->hide(false);
    }
    updatePositions();
}

int PreferenceGroup::indexOf(BaseItem *item)
{
    return itemsExternal_.indexOf(item);
}

const QList<CommonGraphics::BaseItem *> PreferenceGroup::items() const
{
    return itemsExternal_;
}

void PreferenceGroup::updateScaling()
{
    BaseItem::updateScaling();
    updatePositions();
}

void PreferenceGroup::setDescriptionBorderWidth(int width)
{
    borderWidth_ = width;
}

void PreferenceGroup::updatePositions()
{
    // calculate right margin
    if (!error_ && !descUrl_.isEmpty()) {
        descRightMargin_ = (2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE;
    } else {
        descRightMargin_ = PREFERENCES_MARGIN_X*G_SCALE;
    }

    // calculate item positions
    int yPos = 0;
    for (int i = 0; i < items_.size(); i++) {
        items_[i]->setPos(0, yPos);
        yPos += items_[i]->boundingRect().height();
    }

    int descVMargin = 0;
    // calculate description height
    if (descHidden_ || (error_ && errorDesc_.isEmpty()) || (!error_ && desc_.isEmpty())) {
        descHeight_ = 0;
        descVMargin = 0;
    } else {
        descVMargin = (DESCRIPTION_MARGIN + borderWidth_)*G_SCALE;
        if (size() == 0) {
            // use full margin if there are no items in this group
            descVMargin = (PREFERENCES_MARGIN_Y + borderWidth_)*G_SCALE;
        }
        if (error_) {
            QFont font = FontManager::instance().getFont(12,  QFont::Normal);
            QFontMetrics metrics(font);
            descHeight_ = metrics.boundingRect(boundingRect().adjusted((2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE, 0, -descRightMargin_, 0).toRect(),
                                               Qt::AlignLeft | Qt::TextWordWrap,
                                               errorDesc_).height();
        } else {
            QFont font = FontManager::instance().getFont(12,  QFont::Normal);
            QFontMetrics metrics(font);
            descHeight_ = metrics.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -descRightMargin_, 0).toRect(),
                                               Qt::AlignLeft | Qt::TextWordWrap,
                                               desc_).height();
        }
    }

    if (height_ != yPos + descHeight_ + descVMargin) {
        prepareGeometryChange();
        height_ = yPos + descHeight_ + descVMargin;
        if (size() == 0) {
            height_ += PREFERENCES_MARGIN_Y*G_SCALE;
        }
        setHeight(height_);
    }
    update();

    // set description icon location
    if (!error_ && !descUrl_.isEmpty()) {
        icon_->setVisible(true);
        icon_->setPos(boundingRect().width() + PREFERENCES_MARGIN_X*G_SCALE - descRightMargin_,
                      boundingRect().height() - descHeight_/2 - descVMargin - ICON_HEIGHT/2*G_SCALE);
    } else {
        icon_->setVisible(false);
    }
}

void PreferenceGroup::onIconClicked()
{
    QDesktopServices::openUrl(QUrl(descUrl_));
}

int PreferenceGroup::size()
{
    return itemsExternal_.size();
}

void PreferenceGroup::setDescription(const QString &desc, bool error)
{
    if (error) {
        errorDesc_ = desc;
    } else {
        desc_ = desc;
    }
    error_ = error;
    updatePositions();
}

void PreferenceGroup::setDescription(const QString &desc, const QString &descUrl)
{
    desc_ = desc;
    descUrl_ = descUrl;
    updatePositions();
}

void PreferenceGroup::showItems(int start, int end, uint32_t flags)
{
    bool animate = !(flags & DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    int s = INTERNAL_INDEX(start);
    if (s < 0) {
        s = 0;
    }
    int e = (end == -1) ? s : INTERNAL_INDEX(end);
    if (e > items_.size() - 1) {
        e = items_.size() - 1;
    }
    int firstVisible = firstVisibleItem();

    // include preceding divider if 's' is not the first visible item
    if (s > 0 && firstVisible >= 0 && s > firstVisible) {
        s -= 1;
    }

    // show divider for previously first visible item if we are showing items before it
    if (firstVisible > 0 && e < firstVisible && !isDividerLine(firstVisible)) {
        items_[firstVisible - 1]->show(animate);
    }

    for (int i = s; i <= e; i++) {
        // don't show items slated for deletion
        if (!toDelete_.contains(items_[i])) {
            items_[i]->show(animate);
        }
    }
}

void PreferenceGroup::hideItems(int start, int end, uint32_t flags)
{
    bool animate = !(flags & DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    bool deleteAfter = (flags & DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    int s = INTERNAL_INDEX(start);
    if (s < 0) {
        s = 0;
    }
    int e = (end == -1) ? s : INTERNAL_INDEX(end);
    if (e > items_.size() - 1) {
        e = items_.size() - 1;
    }
    int firstVisible = firstVisibleItem();

    // include preceding divider
    if (s > 0) {
        s -= 1;
    }
    // or, if we're hiding from the front of the list, the succeeding one
    // this ensures if we delete these items, that the even items are always real items and odd items are always dividers
    else if (e < items_.size() - 1) {
        e += 1;
    }

    // if we're hiding the first visible item(s), also hide the divider before the next visible item
    if (firstVisible >= s && firstVisible <= e && e < items_.size() - 1) {
        int nextVisible = firstVisibleItem(e + 1);
        if (nextVisible > 0 && isDividerLine(nextVisible)) {
            items_[nextVisible]->hide(animate);
        }
    }

    for (int i = e; i >= s; i--) {
        if (deleteAfter) {
            toDelete_ << items_[i];
        }
        items_[i]->hide(animate);
    }
}

void PreferenceGroup::showDescription()
{
    descHidden_ = false;
    updatePositions();
}

void PreferenceGroup::hideDescription()
{
    descHidden_ = true;
    updatePositions();
}

void PreferenceGroup::clearError()
{
    error_ = false;
    errorDesc_ = "";
    updatePositions();
}

void PreferenceGroup::setDrawBackground(bool draw)
{
    drawBackground_ = draw;
}

int PreferenceGroup::firstVisibleItem(int start)
{
    for (int i = start; i < items_.size(); i++) {
        // Don't check items_[i].isVisible() here, because if the current preferences screen is not visible, all its children will also not be visible
        // This matters if we are on a different screen (say, importing preferences), and it changes the values inside some preference groups
        if (!items_[i]->isHidden()) {
            return i;
        }
    }
    return -1;
}

bool PreferenceGroup::isDividerLine(int index)
{
    // even indexes are items, odd numbers are dividers
    return (index % 2 != 0);
}

} // namespace PreferencesWindow
