#include "subpageitem.h"

#include <QPainter>
#include "GraphicResources/fontmanager.h"
#include "GraphicResources/imageresourcessvg.h"
#include "CommonGraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SubPageItem::SubPageItem(ScalableGraphicsObject *parent, const QString &title, bool clickable) : BaseItem(parent, 50, "", clickable),
    title_(title)
{
    curTextOpacity_ = OPACITY_UNHOVER_TEXT;
    curArrowOpacity_= OPACITY_UNHOVER_ICON_STANDALONE;

    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 47);

    connect(this, SIGNAL(hoverEnter()), SLOT(onHoverEnter()));
    connect(this, SIGNAL(hoverLeave()), SLOT(onHoverLeave()));

    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));
    connect(&arrowOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onArrowOpacityChanged(QVariant)));
}

void SubPageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // text
    painter->setOpacity(curTextOpacity_ * initOpacity);
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 1*G_SCALE, 0, -2*G_SCALE), Qt::AlignVCenter, tr(title_.toStdString().c_str()));

    // arrow
    painter->setOpacity(curArrowOpacity_ * initOpacity);
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/FRWRD_ARROW_WHITE_ICON");
    p->draw(static_cast<int>(boundingRect().width() - p->width() - 16*G_SCALE), static_cast<int>((boundingRect().height() - p->height()) / 2), painter);

    // arrow text
    painter->setOpacity(curTextOpacity_ * initOpacity);
    int arrowTextWidth = CommonGraphics::textWidth(arrowText_, *font);
    int arrowTextPosX = boundingRect().width() - arrowTextWidth - 35*G_SCALE;
    painter->drawText(boundingRect().adjusted(arrowTextPosX, 1*G_SCALE,0, -2*G_SCALE), Qt::AlignVCenter, arrowText_);
}

void SubPageItem::hideOpenPopups()
{
    // overwrite
}

void SubPageItem::setArrowText(QString text)
{
    arrowText_ = text;
    update();
}

void SubPageItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, 47*G_SCALE);
}

void SubPageItem::onHoverEnter()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(arrowOpacityAnimation_, curArrowOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void SubPageItem::onHoverLeave()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(arrowOpacityAnimation_, curArrowOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
}

void SubPageItem::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void SubPageItem::onArrowOpacityChanged(const QVariant &value)
{
    curArrowOpacity_ = value.toDouble();
    update();
}


} // namespace PreferencesWindow
