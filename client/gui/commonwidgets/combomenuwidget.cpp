#include "combomenuwidget.h"

#include <QPainter>
#include <QFocusEvent>
#include <QFont>
#include "commongraphics/commongraphics.h"
#include "utils/makecustomshadow.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

namespace CommonWidgets {

ComboMenuWidget::ComboMenuWidget(QWidget *parent) : QWidget(parent)
  , trackpadDeltaSum_(0)
  , menuListPosY_(0)
{

#ifdef Q_OS_WIN
    // Qt::SubWindow will make windowmanager icon change color -- but it won't interfere with screen resizing properly during a scaling change (Qt::PopUp -- probably qt bug)
    // Qt::SubWindow will also take focus in/out events properly (unlike Qt::Tool and Qt::ToolTip)
    // Until Popup bug is fixed Qt::SubWindow is best option
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#else
    // Menu not visible on Mac with when it is a Qt::SubWindow
    // It appears that Popup is no longer necessary to prevent mainwindow WindowDeactivate event from hiding the app while using the combobox
    // I have removed it since it causes a very rare crash when we attempt to call show() on the menu on MacOS Catalina
    // Seems like a Qt bug -- as of Qt5.12.7 can reproduce unreliably (with popup enabled):
    // 1. go to preferences connection window and open port combobox
    // 2. wheel up/down and select a different port a few times
    // 3. go to preferences general window
    // 4. click on the comboboxes you can see, scroll down click on the rest
    // If it becomes clear that we NEED the Qt::Popup then perhaps this bug will be resolved in future Qt versions.
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#endif
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    //font_ = *FontManager::instance().getFont(12*g_pixelRatio, false);

    layout_ = new QVBoxLayout();
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    layout_->setSizeConstraint(QLayout::SetFixedSize);

    menuListWidget_ = new QWidget(this);
    menuListWidget_->setLayout(layout_);
    menuListWidget_->show();

    scrollBar_ = new VerticalScrollBarWidget(SCROLL_BAR_WIDTH, 90, this);
    connect(scrollBar_, SIGNAL(moved(double)), SLOT(onScrollBarMoved(double)));
    scrollBar_->setOpacity(OPACITY_FULL);
    scrollBar_->setBackgroundColor(Qt::transparent);
    scrollBar_->setForegroundColor(Qt::black);
    scrollBar_->show();

    QString layoutStyleSheet = " border: none; "
                               " background-color: rgba(0,0,0, 0%) ";
    menuListWidget_->setStyleSheet(layoutStyleSheet);
}

void ComboMenuWidget::addItem(QString text, const QVariant &item_data)
{
    ComboMenuWidgetButton *button = new ComboMenuWidgetButton(item_data);
    QFont *font = FontManager::instance().getFont(12, false);
    button->setFont(*font);
    button->setCheckable(true);

    connect(button, SIGNAL(pressed()), SLOT(onButtonClick()));
    connect(button, SIGNAL(hoverEnter()), SLOT(onButtonHoverEnter()));

    items_.append(button);

    QFontMetrics fm(*font);
    button->setWidthUnscaled(fm.boundingRect(button->text()).width() + 30);
    button->setHeightUnscaled(STEP_SIZE);
    button->setText(text);

    layout_->addWidget(button);
    updateScaling();
    updateScrollBarVisibility();

}

void ComboMenuWidget::removeItem(int index)
{
    ComboMenuWidgetButton *button = items_.at(index);

    items_.removeAt(index);
    layout_->removeWidget(button);

    if (button != nullptr)
    {
        delete button;
        button = nullptr;
    }
}

void ComboMenuWidget::navigateItemToTop(int index)
{
    trackpadDeltaSum_ = 0;
    if (index >= 0 && index < items_.count())
    {
        int newValue = -index*STEP_SIZE*G_SCALE;
        moveListPos(newValue);
    }
}

void ComboMenuWidget::activateItem(int index)
{
    if (index >= 0 && index < items_.count())
    {
        ComboMenuWidgetButton *button = items_[index];
        button->setChecked(true); //  use checked to signify the currently selected button
    }
}

void ComboMenuWidget::clearItems()
{
    for (auto item : items_)
    {
        Q_UNUSED(item);
        removeItem(0);
    }
}

int ComboMenuWidget::visibleItems()
{
    if (items_.length() > maxItemsShowing_)
    {
        return maxItemsShowing_;
    }
    else
    {
        return items_.length();
    }
}

int ComboMenuWidget::itemHeight()
{
    return STEP_SIZE * G_SCALE;
}

int ComboMenuWidget::indexOfLastToppableItem()
{
    return items_.length() - visibleItems();
}

QString ComboMenuWidget::textOfItem(int index)
{
    QString text = "";

    if (index >= 0 && index < items_.length())
    {
        text = items_[index]->text();
    }

    return text;
}

void ComboMenuWidget::onButtonClick()
{
    ComboMenuWidgetButton * button = static_cast<ComboMenuWidgetButton *>(sender());

    if (button != nullptr)
    {
        // qCDebug(LOG_USER) << "ComboMenu item clicked: " << button->text();
        emit itemClicked(button->text(), button->data());
    }
}

void ComboMenuWidget::onButtonHoverEnter()
{
    // override any currently selected buttons
    for (auto item : items_)
    {
        item->setChecked(false);
    }
}

QPixmap ComboMenuWidget::getCurrentPixmapShape()
{
    QPixmap tempPixmap(geometry().width(), geometry().height());
    tempPixmap.fill(Qt::transparent);

    QPainter painter(&tempPixmap);
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawRoundedRect(roundedBackgroundRect_, ROUNDED_CORNER, ROUNDED_CORNER, Qt::RelativeSize);
    return tempPixmap;
}

int ComboMenuWidget::itemCount()
{
    return items_.count();
}

void ComboMenuWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // shadow
    QPixmap shape = getCurrentPixmapShape();
    QPixmap shadow = MakeCustomShadow::makeShadowForPixmap(shape, 0, SHADOW_SIZE, Qt::black);

    QPixmap scaledShadow = shadow.scaled(shape.width(), shape.height());
    painter.drawPixmap(0, 0, scaledShadow);

    // background
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawRoundedRect(roundedBackgroundRect_, ROUNDED_CORNER, ROUNDED_CORNER, Qt::RelativeSize);
}

void ComboMenuWidget::keyPressEvent(QKeyEvent *event)
{
    // qCDebug(LOG_USER) << "ComboMenu keypress: " << event->text();
    if (event->key() == Qt::Key_Escape)
    {
        hide();
    }
    else if (event->key() == Qt::Key_R)
    {
        emit requestScalingUpdate();
    }
}

void ComboMenuWidget::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    // qCDebug(LOG_PREFERENCES) << "ComboMenu hide event";
    emit hidden();
}

void ComboMenuWidget::wheelEvent(QWheelEvent *event)
{
    // Some Windows trackpads send multple small deltas instead of one single large delta (like mouse scroll)
    // "Phase" data does not indicate anything helpful, all are "NoScrollPhase", unlike MacOS phase seen above
    // So we track scrolling until it accumulates to similar delta
    int change = event->angleDelta().y();
    if (abs(change) != 120) // must be trackpad
    {
        change += trackpadDeltaSum_;
    }
    else // could be mouse or trackpad
    {
        // if someone starts using mouse (or +/- 120 trackpad) to scoll then clear accumulation
        trackpadDeltaSum_ = 0;
    }

    if (change > TRACKPAD_DELTA_THRESHOLD) // scroll up
    {
        //qCDebug(LOG_USER) << "ComboMenu wheeling (up): " << event->delta();
        int newY = menuListPosY_ + TRACKPAD_DELTA_THRESHOLD; // moves inner down
        trackpadDeltaSum_ = change - TRACKPAD_DELTA_THRESHOLD;
        moveListPos(newY);
    }
    else if (change < -TRACKPAD_DELTA_THRESHOLD) // scroll down
    {
        //qCDebug(LOG_USER) << "ComboMenu wheeling (down): " << event->delta();
        int newY = menuListPosY_ - TRACKPAD_DELTA_THRESHOLD; // moves inner up
        trackpadDeltaSum_ = change + TRACKPAD_DELTA_THRESHOLD;
        moveListPos(newY);
    }
    else // accumulate changes
    {
        trackpadDeltaSum_ = change;
    }
}

void ComboMenuWidget::focusOutEvent(QFocusEvent * /*event*/)
{
    hide();
}

void ComboMenuWidget::onScrollBarMoved(double posPercentY)
{
    // assumes scrollbar will not send strange value
    int posY = posPercentY * menuListWidget_->geometry().height();
    menuListPosY_ = -posY;

    updateScaling();
}

ComboMenuWidgetButton *ComboMenuWidget::itemByName(QString text)
{
    ComboMenuWidgetButton *result = nullptr;

    for (auto item : items_)
    {
        if (text == item->text())
        {
            result = item;
            break;
        }
    }

    return result;
}

void ComboMenuWidget::moveListPos(int newY)
{
    // not too high, not too low
    const int kMinY = qMin( 0, maxViewportHeight_ - menuListWidget_->geometry().height() );
    newY = qBound(kMinY, newY, 0);

    if (menuListPosY_ != newY) {
        menuListPosY_ = newY;
        updateScaling();
    }
}

int ComboMenuWidget::indexOfItemByName(QString text)
{
    int found = -1;
    for (int i = 0; i < items_.length(); i++)
    {
        if (text == items_[i]->text())
        {
            found = i;
            break;
        }
    }

    return found;
}

int ComboMenuWidget::indexByButton(ComboMenuWidgetButton *button)
{
    int found = -1;
    for (int index = 0; index < items_.length(); index++)
    {
        if (items_[index] == button)
        {
            found = index;
            break;
        }
    }
    return found;
}

int ComboMenuWidget::topMargin()
{
    return (SPACER_SIZE + SHADOW_SIZE) * G_SCALE;
}

int ComboMenuWidget::sideMargin()
{
    return SHADOW_SIZE * G_SCALE;
}

void ComboMenuWidget::updateScrollBarVisibility()
{
    if (scrollBarHeightFraction() < .97) // protect from rounding errors
    {
        scrollBar_->show();
    }
    else
    {
        scrollBar_->hide();
    }
}

void ComboMenuWidget::updateScaling()
{
    // update button widths
    QFont *font = FontManager::instance().getFont(12, false);
    for (auto buttonItem : items_)
    {
        ComboMenuWidgetButton * button  = static_cast<ComboMenuWidgetButton *>(buttonItem);
        button->setFont(*font);
        QFontMetrics fm(*font);
        button->setWidthUnscaled(fm.boundingRect(button->text()).width() + 30);
    }

    // restrict height of list viewport
    maxViewportHeight_ = maxItemsShowing_ * STEP_SIZE * G_SCALE;
    int rHeight = restrictedHeight();
    int allButtonsHeightScaled = allButtonsHeight();

    // find appropriate width
    int buttonWidth = largestButtonWidthUnscaled();
    if (buttonWidth < MINIMUM_COMBO_MENU_WIDTH*G_SCALE ) buttonWidth = MINIMUM_COMBO_MENU_WIDTH *G_SCALE ; // minimum width
    if (rHeight < allButtonsHeightScaled) buttonWidth += 25*G_SCALE; // Add width when there is scrollbar to display

    // resize buttons to width
    for (auto buttonItem : items_)
    {
        ComboMenuWidgetButton * button  = static_cast<ComboMenuWidgetButton *>(buttonItem);
        button->setFixedHeight(STEP_SIZE * G_SCALE);
        button->setFixedWidth(buttonWidth);
    }

    // resize list widget
    QRect rect(sideMargin(), menuListPosY_ + topMargin(), buttonWidth, allButtonsHeightScaled);
    menuListWidget_->setGeometry(rect);


    // clip menu list
    clippingRegion_ = QRect(0, -menuListPosY_, buttonWidth, rHeight);
    menuListWidget_->setMask(clippingRegion_);

    // update rounded background size
    roundedBackgroundRect_ = QRect(SHADOW_SIZE*G_SCALE,
                                   SHADOW_SIZE*G_SCALE,
                                   buttonWidth,
                                   rHeight + (SPACER_SIZE*G_SCALE)*2 );

    // update scroll bar
    scrollBar_->setHeight(rHeight);
    scrollBar_->setGeometry(buttonWidth - SCROLL_BAR_WIDTH*G_SCALE, topMargin(), SCROLL_BAR_WIDTH * G_SCALE, rHeight);
    scrollBar_->setBarPortion(scrollBarHeightFraction());
    double yPosPercent = static_cast<double>(menuListPosY_) / menuListWidget_->geometry().height();
    scrollBar_->moveBarToPercentPos(yPosPercent);

    // update main view port
    emit sizeChanged(buttonWidth + sideMargin()*2, rHeight + topMargin()*2 );
}

void ComboMenuWidget::setMaxItemsShowing(int maxItemsShowing)
{
    maxItemsShowing_ = qMax(1, maxItemsShowing);
    updateScaling();
    updateScrollBarVisibility();
}

int ComboMenuWidget::largestButtonWidthUnscaled()
{
    int largestButtonWidth = 0;

    for (auto button : items_)
    {
        if (button->widthUnscaled() > largestButtonWidth)
        {
            largestButtonWidth = button->widthUnscaled();
        }
    }

    return largestButtonWidth;
}

int ComboMenuWidget::allButtonsHeight()
{
    return static_cast<int>(items_.count() * STEP_SIZE *G_SCALE);
}

double ComboMenuWidget::scrollBarHeightFraction()
{
    return static_cast<double>(restrictedHeight())/(allButtonsHeight());
}

int ComboMenuWidget::restrictedHeight()
{
    int restrictedHeight = allButtonsHeight();
    if (restrictedHeight > maxViewportHeight_) restrictedHeight = maxViewportHeight_;
    return restrictedHeight;
}


}
