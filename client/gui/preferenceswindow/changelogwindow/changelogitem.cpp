#include "changelogitem.h"

#include <QFile>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ChangelogItem::ChangelogItem(ScalableGraphicsObject *parent) : CommonGraphics::BaseItem(parent, 0)
{
    QFile file(":changelog.txt");
    if (file.open(QIODevice::ReadOnly))
    {
        text_ = file.readAll();
    }
    file.close();
}

void ChangelogItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(12, false);
    painter->setOpacity(OPACITY_HALF);
    painter->setPen(Qt::white);
    painter->setFont(*font);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::TextWordWrap | Qt::AlignLeft,
                      text_);
}

void ChangelogItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void ChangelogItem::updatePositions()
{
    QFont *font = FontManager::instance().getFont(12, false);
    QFontMetrics fm(*font);
    setHeight(fm.boundingRect(0, 0, boundingRect().width(), INT_MAX, Qt::TextWordWrap | Qt::AlignLeft, text_).height());
    update();
}

} // namespace PreferencesWindow