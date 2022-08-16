#include "titleitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

TitleItem::TitleItem(ScalableGraphicsObject *parent, const QString &title)
  : BaseItem(parent, ACCOUNT_TITLE_HEIGHT*G_SCALE), title_(title)
{
}

void TitleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(11, true);
    qreal oldLetterSpacing = font->letterSpacing();
    font->setLetterSpacing(QFont::AbsoluteSpacing, 2.44);
    painter->setFont(*font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_HALF);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, 0, -PREFERENCES_MARGIN*G_SCALE, 0), Qt::AlignLeft, title_);
    font->setLetterSpacing(QFont::AbsoluteSpacing, oldLetterSpacing);
}

void TitleItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(ACCOUNT_TITLE_HEIGHT*G_SCALE);
}

void TitleItem::setTitle(const QString &title)
{
    title_ = title;
}

} // namespace PreferencesWindow
