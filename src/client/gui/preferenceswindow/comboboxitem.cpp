#include "comboboxitem.h"

#include <QCursor>
#include <QDesktopServices>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>
#include <QUrl>

#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferencesconst.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/ws_assert.h"

namespace PreferencesWindow {

ComboBoxItem::ComboBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip)
  : CommonGraphics::BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), strCaption_(caption), strTooltip_(tooltip),
    captionFont_(14, QFont::Bold), icon_(nullptr), isCaptionElided_(false), inProgress_(false), spinnerRotation_(0)
{
    connect(this, &ComboBoxItem::heightChanged, this, &ComboBoxItem::updatePositions);

    button_ = new CommonGraphics::TextIconButton(4, caption, "preferences/CNTXT_MENU_ICON", this, false);
    button_->setClickable(true);
    connect(button_, &CommonGraphics::TextIconButton::clicked, this, &ComboBoxItem::onMenuOpened);
    button_->setFont(FontDescr(14, QFont::Normal));
    connect(button_, &CommonGraphics::TextIconButton::widthChanged, this, &ComboBoxItem::updatePositions);
    connect(button_, &CommonGraphics::TextIconButton::hoverEnter, this, &ComboBoxItem::buttonHoverEnter);
    connect(button_, &CommonGraphics::TextIconButton::hoverLeave, this, &ComboBoxItem::buttonHoverLeave);

    menu_ = new CommonWidgets::ComboMenuWidget();
    connect(menu_, &CommonWidgets::ComboMenuWidget::itemClicked, this, &ComboBoxItem::onMenuItemSelected);
    connect(menu_, &CommonWidgets::ComboMenuWidget::hidden, this, &ComboBoxItem::onMenuHidden);
    connect(menu_, &CommonWidgets::ComboMenuWidget::sizeChanged, this, &ComboBoxItem::onMenuSizeChanged);

    infoIcon_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/INFO_ICON", "", this, OPACITY_SIXTY);
    connect(infoIcon_, &IconButton::clicked, this, &ComboBoxItem::onInfoIconClicked);

    recalculateCaptionY();

    connect(&captionPosAnimation_, &QVariantAnimation::valueChanged, this, &ComboBoxItem::onCaptionPosUpdated);

    connect(&spinnerAnimation_, &QVariantAnimation::valueChanged, this, &ComboBoxItem::onSpinnerRotationChanged);
    connect(&spinnerAnimation_, &QVariantAnimation::finished, this, &ComboBoxItem::onSpinnerRotationFinished);

    setAcceptHoverEvents(true);
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

    qreal xOffset = PREFERENCES_MARGIN_X*G_SCALE;
    if (icon_)
    {
        xOffset = (PREFERENCES_MARGIN_X + 8 + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    }

    QFontMetrics fm(font);
    int availableWidth = boundingRect().width() - xOffset - button_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE;
    QString elidedText = strCaption_;

    // Reset elided flag
    isCaptionElided_ = false;

    if (availableWidth < fm.horizontalAdvance(strCaption_))
    {
        elidedText = fm.elidedText(strCaption_, Qt::ElideRight, availableWidth, 0);
        isCaptionElided_ = true;
    }

    // Store the caption rectangle for mouse hover detection
    captionRect_ = QRectF(
        xOffset,
        PREFERENCES_ITEM_Y*G_SCALE,
        fm.horizontalAdvance(elidedText),
        fm.height()
    );

    painter->drawText(boundingRect().adjusted(xOffset, curCaptionY_, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignLeft, elidedText);

    // Draw spinner in place of button if in progress
    if (inProgress_) {
        painter->setOpacity(OPACITY_SIXTY);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
        painter->save();
        painter->translate(static_cast<int>(boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE - p->width()/2),
                          static_cast<int>(boundingRect().height()/2));
        painter->rotate(spinnerRotation_);
        p->draw(-p->width()/2, -p->height()/2, painter);
        painter->restore();
        painter->setOpacity(OPACITY_FULL);
    }

    // If there's a description draw it
    if (!desc_.isEmpty()) {
        QFont font = FontManager::instance().getFont(12, QFont::Normal);
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->setOpacity(OPACITY_SIXTY);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - descHeight_,
                                                  -descRightMargin_,
                                                  0), Qt::AlignLeft | Qt::TextWordWrap, desc_);
    }
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
    for (const ComboBoxItemDescr &cb : std::as_const(items_))
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
    recalculateCaptionY();
    update();
}

void ComboBoxItem::clear()
{
    items_.clear();
    menu_->clearItems();
    curItem_.clear();
    button_->setText("");
    update();
}

bool ComboBoxItem::hasItemWithFocus()
{
    return menu_->hasFocus();
}

void ComboBoxItem::hideMenu()
{
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
    menu_->updateScaling();
    recalculateCaptionY();
    updatePositions();
}

void ComboBoxItem::updatePositions()
{
    QFont font = FontManager::instance().getFont(captionFont_);
    int captionWidth = CommonGraphics::textWidth(strCaption_, font);

    int used;
    if (icon_) {
        used = (3*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE + captionWidth;
    }
    else {
        used = 2*PREFERENCES_MARGIN_X*G_SCALE + captionWidth;
    }

    if (desc_.isEmpty()) {
        setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
        infoIcon_->setVisible(false);
    }

    if (useMinimumSize_) {
        // When using minimum size, the spacer width is 4, which the TextIconButton uses to the left of the text as well as between the text and the icon.
        // We want to offset this and put the button exactly PREFERENCES_MARGIN_X from the left edge.
        infoIcon_->setVisible(false);
        button_->setPos((PREFERENCES_MARGIN_X - 4)*G_SCALE, 0);
    } else {
        // Ensure a minimum combobox width so we do not completely hide all of its text.
        // This widget's text (strCaption_) will then elide if need be when we render it in the paint() method.
        button_->setMaxWidth(qMax(boundingRect().width() - used, MIN_COMBOBOX_WIDTH*G_SCALE));
        button_->setPos(boundingRect().width() - button_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE - button_->boundingRect().height()) / 2);
    }

    // Descriptions
    if (desc_.isEmpty()) {
        return;
    }

    descRightMargin_ = PREFERENCES_MARGIN_X*G_SCALE;
    if (!descUrl_.isEmpty()) {
        descRightMargin_ += PREFERENCES_MARGIN_X*G_SCALE + ICON_WIDTH*G_SCALE;
    }

    QFontMetrics fm(FontManager::instance().getFont(12, QFont::Normal));
    descHeight_ = fm.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -descRightMargin_, 0).toRect(),
                                  Qt::AlignLeft | Qt::TextWordWrap,
                                  desc_).height();

    if (descUrl_.isEmpty()) {
        infoIcon_->setVisible(false);
    } else {
        infoIcon_->setVisible(true);
        infoIcon_->setPos(boundingRect().width() + PREFERENCES_MARGIN_X*G_SCALE - descRightMargin_,
                          boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - descHeight_/2 - ICON_HEIGHT*G_SCALE/2);
    }

    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE + descHeight_ + DESCRIPTION_MARGIN*G_SCALE);
    update();
}

void ComboBoxItem::onMenuOpened()
{
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

    menu_->move(point.x() - offsetX, heightCentering);
    menu_->show();
    menu_->setFocus();
}

void ComboBoxItem::onMenuItemSelected(QString text, QVariant data)
{
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

void ComboBoxItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isCaptionElided_ && captionRect_.contains(event->pos())) {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(scenePos()));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.x = globalPt.x() + captionRect_.x() + captionRect_.width()/2;
        ti.y = globalPt.y();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.title = strCaption_;
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }

        TooltipController::instance().showTooltipBasic(ti);
    } else {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    }

    BaseItem::hoverMoveEvent(event);
}

void ComboBoxItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    BaseItem::hoverLeaveEvent(event);
}

void ComboBoxItem::setDescription(const QString &description, const QString &url)
{
    desc_ = description;
    descUrl_ = url;
    updatePositions();
}

void ComboBoxItem::onInfoIconClicked()
{
    QDesktopServices::openUrl(QUrl(descUrl_));
}

void ComboBoxItem::setCaptionY(int y, bool animate)
{
    if (y == -1) {
        recalculateCaptionY();
    } else {
        captionY_ = y;
    }
    if (animate) {
        startAnAnimation(captionPosAnimation_, curCaptionY_, captionY_, ANIMATION_SPEED_VERY_FAST);
    } else {
        curCaptionY_ = captionY_;
        update();
    }
}

void ComboBoxItem::onCaptionPosUpdated(const QVariant &value)
{
    curCaptionY_ = value.toInt();
    update();
}

void ComboBoxItem::setButtonFont(const FontDescr &fontDescr)
{
    button_->setFont(fontDescr);
}

void ComboBoxItem::setButtonIcon(const QString &icon)
{
    button_->setImagePath(icon);
}

void ComboBoxItem::setUseMinimumSize(bool useMinimumSize)
{
    useMinimumSize_ = useMinimumSize;
    if (useMinimumSize_) {
        width_ = (PREFERENCES_MARGIN_X - 4) + button_->boundingRect().width()/G_SCALE;
    } else {
        width_ = PAGE_WIDTH;
    }
    prepareGeometryChange();
    update();
}

int ComboBoxItem::buttonWidth() const
{
    return button_->boundingRect().width();
}

void ComboBoxItem::setInProgress(bool inProgress)
{
    inProgress_ = inProgress;
    if (inProgress_) {
        button_->setVisible(false);
        spinnerRotation_ = 0;
        startAnAnimation<int>(spinnerAnimation_, spinnerRotation_, 360, ANIMATION_SPEED_VERY_SLOW);
    } else {
        button_->setVisible(true);
        spinnerAnimation_.stop();
        update();
    }
}

void ComboBoxItem::onSpinnerRotationChanged(const QVariant &value)
{
    spinnerRotation_ = value.toInt();
    update();
}

void ComboBoxItem::onSpinnerRotationFinished()
{
    startAnAnimation<int>(spinnerAnimation_, 0, 360, ANIMATION_SPEED_VERY_SLOW);
}

void ComboBoxItem::recalculateCaptionY()
{
    QFont font = FontManager::instance().getFont(captionFont_);
    QFontMetrics fm(font);
    curCaptionY_ = (PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE - fm.height()) / 2;
    captionY_ = curCaptionY_;
}

} // namespace PreferencesWindow
