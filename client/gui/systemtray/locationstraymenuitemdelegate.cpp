#include "locationstraymenuitemdelegate.h"
#include "locationstraymenuwidget.h"
#include "locationstraymenuscalemanager.h"
#include "graphicresources/imageresourcessvg.h"

#include <QPainter>
#include <QApplication>
#include <QDebug>

LocationsTrayMenuItemDelegate::LocationsTrayMenuItemDelegate(QObject *parent) : QItemDelegate(parent)
{

}

LocationsTrayMenuItemDelegate::~LocationsTrayMenuItemDelegate()
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

    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    const int flags = index.model()->data(index, LocationsTrayMenuWidget::USER_ROLE_FLAGS).toInt();
    const bool bEnabled = flags & LocationsTrayMenuWidget::ITEM_FLAG_IS_ENABLED;
    QSharedPointer<IndependentPixmap> flag = nullptr;

    if (flags & LocationsTrayMenuWidget::ITEM_FLAG_HAS_COUNTRY) {
        flag = ImageResourcesSvg::instance().getScaledFlag(
            index.model()->data(index, LocationsTrayMenuWidget::USER_ROLE_COUNTRY_CODE).toString(),
            20 * LocationsTrayMenuScaleManager::instance().scale(), 10 * LocationsTrayMenuScaleManager::instance().scale(), bEnabled ? 0 : ImageResourcesSvg::IMAGE_FLAG_GRAYED);
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
            bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Base));
    }

    int leftOffs = 10 * LocationsTrayMenuScaleManager::instance().scale();
    if (flag) {
        const int pixmapFlagHeight = flag->height();
        flag->draw(leftOffs, rc.top() + (rc.height() - pixmapFlagHeight) / 2, painter);
        leftOffs += flag->width() + 10 * LocationsTrayMenuScaleManager::instance().scale();
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

    rc.setLeft(rc.left() + leftOffs);
    QTextOption to;
    to.setAlignment(Qt::AlignVCenter);
    painter->setFont(font_);
    painter->drawText(rc, text, to);

    if (flags & LocationsTrayMenuWidget::ITEM_FLAG_HAS_SUBMENU) {
        QStyleOption ao(option);
        ao.rect.setLeft(ao.rect.right() - 20 * LocationsTrayMenuScaleManager::instance().scale());
        ao.palette.setCurrentColorGroup(bEnabled ? QPalette::Active : QPalette::Disabled);
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &ao, painter);
    }
}

QSize LocationsTrayMenuItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionMenuItem opt;
    QSize sz(0,0);
    QSize sz2 = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    int menuHeight = sz2.height() * LocationsTrayMenuScaleManager::instance().scale();

    sz = QItemDelegate::sizeHint(option, index);
    sz.setHeight(menuHeight);
    return sz;
}

int LocationsTrayMenuItemDelegate::calcWidth(const QString &text, const QString &country, int flags) const
{
    QSharedPointer<IndependentPixmap> flag = nullptr;
    if (flags & LocationsTrayMenuWidget::ITEM_FLAG_HAS_COUNTRY) {
        flag = ImageResourcesSvg::instance().getScaledFlag( country,
            20 * LocationsTrayMenuScaleManager::instance().scale(), 10 * LocationsTrayMenuScaleManager::instance().scale(), 0);
    }

    int width = 10 * LocationsTrayMenuScaleManager::instance().scale();
    if (flag)
    {
        width += flag->width() + 10 * LocationsTrayMenuScaleManager::instance().scale();
    }

    QFontMetrics fm(font_);
    QRect rc = fm.boundingRect(text);
    width += rc.width();

    if (flags & LocationsTrayMenuWidget::ITEM_FLAG_HAS_SUBMENU)
    {
        width += 20 * LocationsTrayMenuScaleManager::instance().scale();
    }
    width += 10 * LocationsTrayMenuScaleManager::instance().scale();
    return width;
}
