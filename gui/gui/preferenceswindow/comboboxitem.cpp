#include "comboboxitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include "utils/makecustomshadow.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

namespace PreferencesWindow {

ComboBoxItem::ComboBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip, int height, QColor fillColor, int captionOffsX, bool bShowDividerLine, QString id) : BaseItem(parent, height, id),
    strCaption_(caption), strTooltip_(tooltip), fillColor_(fillColor), captionOffsX_(captionOffsX),
    captionFont_(12, true), dividerLine_(NULL)
{
    initialHeight_ = height;

    button_ = new CommonGraphics::TextIconButton(8, caption, "preferences/CNTXT_MENU_ICON", this, false);
    button_->setClickable(true);
    connect(button_, SIGNAL(clicked()), SLOT(onMenuOpened()));
    button_->setFont(FontDescr(12, false));
    button_->setPos(boundingRect().width()  - button_->boundingRect().width() - 12, (boundingRect().height() - 3 - button_->boundingRect().height()) / 2);
    connect(button_, SIGNAL(widthChanged(int)), this, SLOT(onButtonWidthChanged(int)));
    connect(button_, SIGNAL(hoverEnter()), SIGNAL(buttonHoverEnter()));
    connect(button_, SIGNAL(hoverLeave()), SIGNAL(buttonHoverLeave()));

    menu_ = new CommonWidgets::ComboMenuWidget();
    connect(menu_, SIGNAL(itemClicked(QString,QVariant)), SLOT(onMenuItemSelected(QString, QVariant)));
    connect(menu_, SIGNAL(hidden()), SLOT(onMenuHidden()));
    connect(menu_, SIGNAL(sizeChanged(int, int)), SLOT(onMenuSizeChanged(int,int)));

    if (bShowDividerLine)
    {
        dividerLine_ = new DividerLine(this, 276);
        dividerLine_->setPos(24, height - dividerLine_->boundingRect().height());
    }
}

ComboBoxItem::~ComboBoxItem()
{
    menu_->disconnect();
    delete menu_;
    menu_ = nullptr;
}

void ComboBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    if (fillColor_ != Qt::transparent)
    {
        painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(fillColor_));
    }

    QFont *font = FontManager::instance().getFont(captionFont_);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

    painter->drawText(boundingRect().adjusted((16 + captionOffsX_)*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr(strCaption_.toStdString().c_str()));
}

bool ComboBoxItem::hasItems()
{
    return !items_.isEmpty();
}

void ComboBoxItem::addItem(const QString &caption, const QVariant &userValue)
{
    items_ << ComboBoxItemDescr(caption, userValue);

    menu_->addItem(caption, userValue);

}

void ComboBoxItem::removeItem(const QVariant &value)
{
    bool bNeedUpdate = false;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i].userValue() == value)
        {
            bNeedUpdate = (items_[i] == curItem_);
            items_.removeAt(i);
            menu_->removeItem(i);
            break;
        }
    }
    if (bNeedUpdate)
    {
        if (items_.count() > 0)
        {
            setCurrentItem(items_[0].userValue());
        }
    }
}

void ComboBoxItem::setCurrentItem(QVariant value)
{
    Q_FOREACH(const ComboBoxItemDescr &cb, items_)
    {
        if (cb.userValue() == value)
        {
            curItem_ = cb;
            button_->setText(curItem_.caption());
            update();
            return;
        }
    }

    // if not found, then select first item
    if (items_.count() > 0)
    {
        curItem_ = items_.first();
        button_->setText(curItem_.caption());
        update();
    }
    else
    {
        // should not get here, this is error
        Q_ASSERT(false);
        curItem_.clear();
        update();
    }
}

QVariant ComboBoxItem::currentItem() const
{
    return curItem_.userValue();
}

QString ComboBoxItem::caption() const
{
    return curItem_.caption();
}

void ComboBoxItem::setLabelCaption(QString text)
{
    strCaption_ = text;
    update();
}

QString ComboBoxItem::labelCaption() const
{
    return strCaption_;
}

void ComboBoxItem::setCaptionFont(const FontDescr &fontDescr)
{
    captionFont_ = fontDescr;
}

void ComboBoxItem::clear()
{
    items_.clear();
    menu_->clearItems();
    curItem_.clear();
    update();
}

bool ComboBoxItem::hasMenuWithFocus()
{
    return menu_->hasFocus();
}

void ComboBoxItem::hideMenu()
{
    // qCDebug(LOG_PREFERENCES) << "Hiding menu for: " << strCaption_;
    menu_->hide();
}

void ComboBoxItem::setMaxMenuItemsShowing(int maxItemsShowing)
{
    menu_->setMaxItemsShowing(maxItemsShowing);
}

QPointF ComboBoxItem::getButtonScenePos() const
{
    return button_->scenePos();
}


void ComboBoxItem::setColorScheme(bool /*darkMode*/)
{

}

void ComboBoxItem::setClickable(bool clickable)
{
    button_->setClickableHoverable(clickable, true);
    BaseItem::setClickable(clickable);
}

void ComboBoxItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(initialHeight_*G_SCALE);

    button_->setPos(boundingRect().width()  - button_->boundingRect().width() - 12*G_SCALE, (boundingRect().height() - 3*G_SCALE - button_->boundingRect().height()) / 2);

    if (dividerLine_)
    {
        dividerLine_->setPos(24*G_SCALE, initialHeight_*G_SCALE - dividerLine_->boundingRect().height());
    }
    menu_->updateScaling();
}

void ComboBoxItem::onMenuOpened()
{
    qCDebug(LOG_USER) << "ComboButton clicked: " << strCaption_;
    QPointF buttonTopRight = QPointF(button_->scenePos().x() + button_->boundingRect().width(), button_->scenePos().y());
    QPointF alignedOrigin = QPointF(buttonTopRight.x() - menu_->sizeHint().width(), buttonTopRight.y());

    QGraphicsView *view = scene()->views().first();
    QPoint point = view->mapToGlobal(view->mapFromScene(alignedOrigin)); // for global coords

    QString itemName = curItem_.caption();

    int offsetX = menu_->geometry().width();
    int offsetY = 5;
    int heightCentering = point.y() - menu_->topMargin() - offsetY; // init top item

    int item = menu_->indexOfItemByName(itemName);
    int numItems = menu_->itemCount();

    if (numItems <= menu_->visibleItems()) //  all showing
    {
        // center on selected item
        heightCentering -= item * menu_->itemHeight();
        menu_->navigateItemToTop(0);
    }
    else // scrollbar on
    {
        int offBy = 0;
        if (item == numItems - 1) // last item
        {
            heightCentering  -= 4 * menu_->itemHeight();
        }
        else if (item == numItems - 2) //
        {
            heightCentering -= 3 * menu_->itemHeight();
        }
        else if (item == 0)
        {
            // do nothing
        }
        else if (item == 1)
        {
            heightCentering -= 1 * menu_->itemHeight();
            offBy = 1;
        }
        else
        {
            heightCentering -= 2 * menu_->itemHeight();
            offBy = 2;
        }

        menu_->navigateItemToTop(item - offBy);
        menu_->activateItem(item);
    }

    // Ensure popup visibility when close to the bottom of the current screen.
    const int menu_height = qMin(numItems, menu_->visibleItems()) * menu_->itemHeight()
                            + menu_->topMargin() * 2;
    const int screen_max_ypos = DpiScaleManager::instance().curScreenGeometry().bottom();
    if (heightCentering + menu_height > screen_max_ypos)
        heightCentering = screen_max_ypos - menu_height;

    qCDebug(LOG_PREFERENCES) << "Showing menu: " << strCaption_;
    menu_->move(point.x() - offsetX, heightCentering);
    menu_->show();
    menu_->setFocus();
}

void ComboBoxItem::onMenuItemSelected(QString text, QVariant data)
{
    qCDebug(LOG_PREFERENCES) << "Menu item selected: " << text;
    QVariant newItem = data;
    if (newItem != curItem_.userValue())
    {
        curItem_ = ComboBoxItemDescr(text, newItem);
        button_->setText(curItem_.caption());

        emit currentItemChanged(newItem);
        update();
    }

    hideMenu();
}

void ComboBoxItem::onMenuHidden()
{
    // this fires twice on Mac on off-click
    qCDebug(LOG_PREFERENCES) << "Menu hidden: " << strCaption_;
    parentItem()->setFocus();
}

void ComboBoxItem::onMenuSizeChanged(int width, int height)
{
    menu_->setGeometry(menu_->geometry().x(), menu_->geometry().y(), width, height);
}

void ComboBoxItem::onButtonWidthChanged(int newWidth)
{
    Q_UNUSED(newWidth);

    button_->setPos(boundingRect().width()  - button_->boundingRect().width() - 12,
                    (boundingRect().height() - 3 - button_->boundingRect().height()) / 2);
}

} // namespace PreferencesWindow
