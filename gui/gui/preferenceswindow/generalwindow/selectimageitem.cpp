#include "selectimageitem.h"

#include <QPainter>
#include "../basepage.h"
#include "graphicresources/fontmanager.h"
#include <time.h>
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SelectImageItem::SelectImageItem(ScalableGraphicsObject *parent, const QString &caption, bool bShowDividerLine) :
    ScalableGraphicsObject(parent),
    caption_(caption),
    dividerLine_(nullptr)
{
    if (bShowDividerLine)
    {
        dividerLine_ = new DividerLine(this, 264);
    }

    btnOpen_ = new IconButton(16, 16, "locations/EDIT_ICON", this);
    connect(btnOpen_, SIGNAL(clicked()), SLOT(onOpenClick()));

    path_ = "filename1.png";

    updatePositions();
}

QRectF SelectImageItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, (BODY_HEIGHT + TITLE_HEIGHT)*G_SCALE);
}

void SelectImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), Qt::red);

    qreal initOpacity = painter->opacity();
    painter->setOpacity(0.5 * initOpacity);

    // draw caption
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

    QRectF rcText = boundingRect().adjusted(24*G_SCALE, 0, -16*G_SCALE, -BODY_HEIGHT*G_SCALE);
    painter->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter, caption_);
    font = FontManager::instance().getFont(12, false);
    painter->setFont(*font);
    painter->drawText(rcText, Qt::AlignRight | Qt::AlignVCenter, "664x352");


    painter->setOpacity(initOpacity);

    // draw background for the path
    painter->fillRect(boundingRect().adjusted(24*G_SCALE, TITLE_HEIGHT*G_SCALE, 0, 0), QBrush(QColor(16, 22, 40)));

    // draw the path
    font = FontManager::instance().getFont(12, false);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

    rcText = boundingRect().adjusted((24 + 8)*G_SCALE, TITLE_HEIGHT*G_SCALE, -16*G_SCALE*2 -8*G_SCALE, -3*G_SCALE);
    QFontMetrics fm(*font);
    QString elidedPath = fm.elidedText(path_, Qt::ElideRight, rcText.width());
    painter->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter, elidedPath);
}

void SelectImageItem::setPath(const QString &path)
{
    path_ = path;
    update();
}

void SelectImageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void SelectImageItem::onOpenClick()
{

}


void SelectImageItem::updatePositions()
{
    if (dividerLine_)
    {
        dividerLine_->setPos(24*G_SCALE, (BODY_HEIGHT+TITLE_HEIGHT-3)*G_SCALE);
    }

    int dividerHeight = 2*G_SCALE;

    QRectF rcBody = boundingRect().adjusted(0, TITLE_HEIGHT*G_SCALE, 0, 0);
    btnOpen_->setPos(rcBody.width() - btnOpen_->boundingRect().width() - 16*G_SCALE,
                       (rcBody.height() - dividerHeight - btnOpen_->boundingRect().height()) / 2 + TITLE_HEIGHT*G_SCALE);
}

} // namespace PreferencesWindow

