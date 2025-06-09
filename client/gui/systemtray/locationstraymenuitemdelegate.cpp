#include "locationstraymenuitemdelegate.h"

#include <QPainter>
#include <QApplication>

#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "locations/locationsmodel_roles.h"
#include "types/locationid.h"

LocationsTrayMenuItemDelegate::LocationsTrayMenuItemDelegate(QObject *parent, double scale) : QItemDelegate(parent), scale_(scale)
{
}

void LocationsTrayMenuItemDelegate::setFontForItems(const QFont &font)
{
    font_ = font;
}

void LocationsTrayMenuItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    QString text = index.model()->data(index).toString();
    QString nick = index.model()->data(index, gui_locations::kDisplayNickname).toString();
    if (!nick.isEmpty()) {
        text += " - " + nick;
    }

    if (index.data(gui_locations::kIsShowAsPremium).toBool())
    {
        text += " (Pro)";
    }

    bool bEnabled = index.flags() & Qt::ItemIsEnabled;
    QSharedPointer<IndependentPixmap> flag = nullptr;
    LocationID lid = qvariant_cast<LocationID>(index.data(gui_locations::kLocationId));
    if (!lid.isCustomConfigsLocation()) {
        flag = ImageResourcesSvg::instance().getScaledFlag(
            index.data(gui_locations::kCountryCode).toString(),
            20 * scale_, 10 * scale_, bEnabled ? 0 : ImageResourcesSvg::IMAGE_FLAG_GRAYED);
    }
    QRect rc = option.rect;
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(rc, option.palette.brush(
            bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Highlight));
    }
    else
    {
        painter->fillRect(rc, option.palette.brush(
            bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Window));
    }

    int leftOffs = 10 * scale_;
    if (flag) {
        const int pixmapFlagHeight = flag->height();
        flag->draw(leftOffs, rc.top() + (rc.height() - pixmapFlagHeight) / 2, painter);
        leftOffs += flag->width() + 10 * scale_;
    }

    if (option.state & QStyle::State_Selected)
    {
        painter->setPen(option.palette.color(
            bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::HighlightedText));
    }
    else
    {
        painter->setPen(option.palette.color(
            bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Text));
    }

    text = CommonGraphics::maybeTruncatedText(text, font_, static_cast<int>(CITY_CAPTION_MAX_WIDTH * scale_));
    rc.setLeft(rc.left() + leftOffs);
    QTextOption to;
    to.setAlignment(Qt::AlignVCenter);
    painter->setFont(font_);
    painter->drawText(rc, text, to);

    if (index.model()->rowCount(index) > 0)
    {
        QStyleOption ao(option);
        ao.rect.setLeft(ao.rect.right() - 20 * scale_);
        ao.palette.setCurrentColorGroup(bEnabled ? QPalette::Active : QPalette::Disabled);
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &ao, painter);
    }
}

QSize LocationsTrayMenuItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionMenuItem opt;
    QSize sz(0,0);
    QSize sz2 = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    int menuHeight = sz2.height() * scale_;

    sz = QItemDelegate::sizeHint(option, index);
    sz.setHeight(menuHeight);
    return sz;
}

int LocationsTrayMenuItemDelegate::calcWidth(const QModelIndex & index) const
{
    QSharedPointer<IndependentPixmap> flag = nullptr;
    LocationID lid = qvariant_cast<LocationID>(index.model()->data(index, gui_locations::kLocationId));
    if (!lid.isCustomConfigsLocation()) {
        flag = ImageResourcesSvg::instance().getScaledFlag(
            index.model()->data(index, gui_locations::kCountryCode).toString(),
            20 * scale_, 10 * scale_, 0);
    }

    int width = 10 * scale_;
    if (flag)
    {
        width += flag->width() + 10 * scale_;
    }

    QString text = index.model()->data(index).toString();
    QString nick = index.model()->data(index, gui_locations::kDisplayNickname).toString();
    if (!nick.isEmpty()) {
        text += " - " + nick;
    }

    text = CommonGraphics::maybeTruncatedText(text, font_, static_cast<int>(CITY_CAPTION_MAX_WIDTH * scale_));
    QFontMetrics fm(font_);
    QRect rc = fm.boundingRect(text);
    width += rc.width();

    if (index.model()->rowCount(index) > 0)
    {
        width += 20 * scale_;
    }
    width += 10 * scale_;
    return width;
}
