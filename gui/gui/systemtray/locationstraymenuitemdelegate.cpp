#include "locationstraymenuitemdelegate.h"

#include <QPainter>
#include <QApplication>
#include "dpiscalemanager.h"

LocationsTrayMenuItemDelegate::LocationsTrayMenuItemDelegate(QObject *parent)
{
    QStyleOptionMenuItem opt;
    QSize sz;
    sz = QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz);
    menuHeight_ = sz.height();
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
    bool bEnabled = index.model()->data(index, Qt::UserRole + 1).toBool();

    QRect rc = option.rect;
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(rc, option.palette.brush(bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Highlight));
        painter->setPen(option.palette.color(bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::HighlightedText));
    }
    else
    {
        painter->fillRect(rc, option.palette.brush(bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Background));
        painter->setPen(option.palette.color(bEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Text));
    }

    rc.setLeft(rc.left() + 10 * G_SCALE);
    QTextOption to;
    to.setAlignment(Qt::AlignVCenter);
    painter->setFont(font_);
    painter->drawText(rc, text, to);
}

QSize LocationsTrayMenuItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QItemDelegate::sizeHint(option, index);
    sz.setHeight(menuHeight_);
    return sz;
}
