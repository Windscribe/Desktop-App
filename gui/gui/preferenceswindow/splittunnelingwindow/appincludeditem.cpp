#include "appincludeditem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/widgetutils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AppIncludedItem::AppIncludedItem(ProtoTypes::SplitTunnelingApp app, QString iconPath, ScalableGraphicsObject *parent) : BaseItem (parent, 50)
    , appIcon_(iconPath)
{
    app_ = app;

    deleteButton_ = new IconButton(16, 16, "preferences/DELETE_ICON", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    deleteButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    deleteButton_->setHoverOpacity(OPACITY_FULL);
    connect(deleteButton_, SIGNAL(clicked()), SIGNAL(deleteClicked()));
    connect(deleteButton_, SIGNAL(hoverEnter()), SLOT(onDeleteButtonHoverEnter()));
    connect(deleteButton_, SIGNAL(hoverLeave()), SIGNAL(hoverLeave()));

    line_ = new DividerLine(this, 276);
    updatePositions();
}

AppIncludedItem::~AppIncludedItem()
{
    delete line_;
}

void AppIncludedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // app icon
    painter->save();
    IndependentPixmap *p = ImageResourcesSvg::instance().getIconIndependentPixmap(appIcon_);
    if (p)
    {
#ifdef Q_OS_WIN
        int size = 16*G_SCALE;
#elif defined Q_OS_MAC
        int size = 32;
#endif
        QPixmap drawingPixmap = p->getScaledPixmap(size, size);
        painter->drawPixmap(16*G_SCALE, 12*G_SCALE, drawingPixmap);
    }
    else
    {
        IndependentPixmap *ip = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
        ip->draw(16*G_SCALE, 12*G_SCALE, painter);
    }
    painter->restore();

    // app name
    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    QRect textRect(40*G_SCALE, 13*G_SCALE, 200*G_SCALE, 50*G_SCALE);
    painter->drawText(textRect, QString::fromStdString(app_.name()));
}

QString AppIncludedItem::getName()
{
    return QString::fromStdString(app_.name());
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
    setHeight(50*G_SCALE);
    updatePositions();
}

void AppIncludedItem::onDeleteButtonHoverEnter()
{
    if (!selected_)
    {
        selected_ = true;
        emit selectionChanged(true);
    }
}

void AppIncludedItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - 32*G_SCALE, 50/2*G_SCALE - 14*G_SCALE);
    line_->setPos(24*G_SCALE, 43*G_SCALE);
}

}
