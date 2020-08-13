#include "appsearchitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/widgetutils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AppSearchItem::AppSearchItem(ProtoTypes::SplitTunnelingApp app, QString appIconPath, ScalableGraphicsObject *parent) : BaseItem (parent, 50*G_SCALE)
    , textOpacity_(OPACITY_UNHOVER_TEXT)
    , app_(app)
    , appIcon_(appIconPath)
{
    if (appIcon_ == "")
    {
        appIcon_ = "preferences/WHITE_QUESTION_MARK_ICON";
    }

    line_ = new DividerLine(this, 276);
    line_->setPos(24*G_SCALE, 43*G_SCALE);

    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));
    connect(this, SIGNAL(hoverEnter()), SLOT(onHoverEnter()));

    updateIcons();
}

AppSearchItem::~AppSearchItem()
{
    delete line_;
}

void AppSearchItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
        int size = p->originalPixmapSize().width();
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

    // checkmark icon
    painter->setOpacity(initOpacity * enabledIconOpacity_);
    IndependentPixmap *enabledPixmap = ImageResourcesSvg::instance().getIndependentPixmap(enabledIcon_);
    enabledPixmap->draw(boundingRect().width() - 36*G_SCALE, 12*G_SCALE, painter);

    // text
    painter->setOpacity(textOpacity_ * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    painter->drawText(boundingRect().adjusted(40*G_SCALE, 13*G_SCALE, -48*G_SCALE, -18*G_SCALE), QString::fromStdString(app_.name()));
}

QString AppSearchItem::getName()
{
    return QString::fromStdString(app_.name());
}

QString AppSearchItem::getFullName()
{
    return QString::fromStdString(app_.full_name());
}

QString AppSearchItem::getAppIcon()
{
    return appIcon_;
}

void AppSearchItem::updateIcons()
{
    if (app_.active())
    {
        enabledIcon_ = "preferences/GREEN_CHECKMARK_ICON";
        enabledIconOpacity_ = OPACITY_FULL;
    }
    else
    {
        enabledIcon_ = "preferences/WHITE_CHECKMARK_ICON";
        enabledIconOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    }
}

void AppSearchItem::setSelected(bool selected)
{
    selected_ = selected;

    double targetOpacity = OPACITY_FULL;
    if (!selected) targetOpacity = OPACITY_UNHOVER_TEXT;
    startAnAnimation<double>(textOpacityAnimation_, textOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void AppSearchItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    line_->setPos(24*G_SCALE, 43*G_SCALE);
}

void AppSearchItem::onTextOpacityChanged(const QVariant &value)
{
    textOpacity_ = value.toDouble();
    update();
}

void AppSearchItem::onHoverEnter()
{
    startAnAnimation<double>(textOpacityAnimation_, textOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void AppSearchItem::setAppIcon(QString icon)
{
    appIcon_ = icon;
    update();
}

}
