#include "comboboxitem.h"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>

#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferencesconst.h"
#include "utils/ws_assert.h"

namespace PreferencesWindow {

ComboBoxItem::ComboBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip)
  : CommonGraphics::BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), strCaption_(caption), strTooltip_(tooltip),
    captionFont_(12, true), icon_(nullptr)
{
    connect(this, &ComboBoxItem::heightChanged, this, &ComboBoxItem::updatePositions);

    button_ = new CommonGraphics::TextIconButton(8, caption, "preferences/CNTXT_MENU_ICON", this, false);
    button_->setClickable(true);
    connect(button_, &CommonGraphics::TextIconButton::clicked, this, &ComboBoxItem::onMenuOpened);
    button_->setFont(FontDescr(12, false));
    connect(button_, &CommonGraphics::TextIconButton::widthChanged, this, &ComboBoxItem::updatePositions);
    connect(button_, &CommonGraphics::TextIconButton::hoverEnter, this, &ComboBoxItem::buttonHoverEnter);
    connect(button_, &CommonGraphics::TextIconButton::hoverLeave, this, &ComboBoxItem::buttonHoverLeave);

    menu_ = new CommonWidgets::ComboMenuWidget();
    connect(menu_, &CommonWidgets::ComboMenuWidget::itemClicked, this, &ComboBoxItem::onMenuItemSelected);
    connect(menu_, &CommonWidgets::ComboMenuWidget::hidden, this, &ComboBoxItem::onMenuHidden);
    connect(menu_, &CommonWidgets::ComboMenuWidget::sizeChanged, this, &ComboBoxItem::onMenuSizeChanged);

    updateScaling();
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

    QFont font = FontManager::instance().getFont(captionFont_);
    painter->setFont(font);
    painter->setPen(Qt::white);

    qreal xOffset = PREFERENCES_MARGIN*G_SCALE;
    if (icon_)
    {
        xOffset = (1.5*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    }

    QFontMetrics fm(font);
    int availableWidth = boundingRect().width() - xOffset - button_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE;
    QString elidedText = strCaption_;
    if (availableWidth < fm.horizontalAdvance(strCaption_))
    {
        elidedText = fm.elidedText(strCaption_, Qt::ElideRight, availableWidth, 0);
    }

    painter->drawText(boundingRect().adjusted(xOffset,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::AlignLeft,
                      elidedText);
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
    for (const ComboBoxItemDescr &cb : qAsConst(items_))
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
        WS_ASSERT(false);
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

bool ComboBoxItem::hasItemWithFocus()
{
    return menu_->hasFocus();
}

void ComboBoxItem::hideMenu()
{
    // qCDebug(LOG_PREFERENCES) << "Hiding menu for: " << strCaption_;
    menu_->hide();

#ifdef Q_OS_MACOS
    // HACK: There is a Qt issue introduced in Qt 6.5 for MacOS only, where after clicking
    // anything in the menu, the hover effects from its stylesheet no longer apply.
    // As a workaround, recreate and repopulate the menu item every time we hide it.

    menu_->deleteLater();

    menu_ = new CommonWidgets::ComboMenuWidget();
    connect(menu_, &CommonWidgets::ComboMenuWidget::itemClicked, this, &ComboBoxItem::onMenuItemSelected);
    connect(menu_, &CommonWidgets::ComboMenuWidget::sizeChanged, this, &ComboBoxItem::onMenuSizeChanged);
    connect(menu_, &CommonWidgets::ComboMenuWidget::hidden, this, &ComboBoxItem::onMenuHidden);

    for (auto item : items_) {
        menu_->addItem(item.caption(), item.userValue());
    }
#endif
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
}

void ComboBoxItem::updateScaling()
{
    CommonGraphics::BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    menu_->updateScaling();
    updatePositions();
}

void ComboBoxItem::updatePositions()
{
    QFont font = FontManager::instance().getFont(captionFont_);
    int captionWidth = CommonGraphics::textWidth(strCaption_, font);

    int used;
    if (icon_) {
        used = (3*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE + captionWidth;
    }
    else {
        used = 2*PREFERENCES_MARGIN*G_SCALE + captionWidth;
    }

    // Ensure a minimum combobox width so we do not completely hide all of its text.
    // This widget's text (strCaption_) will then elide if need be when we render it in the paint() method.
    button_->setMaxWidth(qMax(boundingRect().width() - used, MIN_COMBOBOX_WIDTH*G_SCALE));
    button_->setPos(boundingRect().width() - button_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
}

void ComboBoxItem::onMenuOpened()
{
    // qCDebug(LOG_USER) << "ComboButton clicked: " << strCaption_;
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

    // qCDebug(LOG_PREFERENCES) << "Showing menu: " << strCaption_;
    menu_->move(point.x() - offsetX, heightCentering);
    menu_->show();
    menu_->setFocus();
}

void ComboBoxItem::onMenuItemSelected(QString text, QVariant data)
{
    // qCDebug(LOG_PREFERENCES) << "Menu item selected: " << text;
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
    // qCDebug(LOG_PREFERENCES) << "Menu hidden: " << strCaption_;
    parentItem()->setFocus();
}

void ComboBoxItem::onMenuSizeChanged(int width, int height)
{
    menu_->setGeometry(menu_->geometry().x(), menu_->geometry().y(), width, height);
}

void ComboBoxItem::setIcon(QSharedPointer<IndependentPixmap> icon)
{
    icon_ = icon;
}

void ComboBoxItem::setItems(const QList<QPair<QString, QVariant>> &list, QVariant curValue)
{
    clear();

    for (const auto item : list) {
        addItem(item.first, item.second);
    }
    setCurrentItem(curValue);
    updatePositions();
}

} // namespace PreferencesWindow
