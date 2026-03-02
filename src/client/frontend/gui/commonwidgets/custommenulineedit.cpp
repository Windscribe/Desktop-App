#include "custommenulineedit.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMimeData>
#include <QStyle>
#include "dpiscalemanager.h"

// #include <QDebug>

namespace CommonWidgets {

CustomMenuLineEdit::CustomMenuLineEdit(QWidget *parent) : BlockableQLineEdit (parent),
    icon_(nullptr), icon1_(nullptr), icon2_(nullptr), echoMode_(QLineEdit::Normal), showRevealToggle_(false), isRevealed_(false)
{
    setContextMenuPolicy(Qt::NoContextMenu);

    menu_ = new CustomMenuWidget();
    menu_->setColorScheme(false);
    menu_->initContextMenu();
    connect(menu_, &CustomMenuWidget::triggered, this, &CustomMenuLineEdit::onMenuTriggered);
}

void CustomMenuLineEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        updateActionsState();
        QRect globalRect = QRect(mapToGlobal(rect().topLeft()), size());
        if (globalRect.contains(QCursor::pos())) {
            menu_->exec(QCursor::pos());
        }
    } else {
        BlockableQLineEdit::mousePressEvent(event);
    }
}

void CustomMenuLineEdit::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange)
    {
        menu_->clearItems();
        menu_->clear();
        menu_->initContextMenu();
    }
    else
    {
        BlockableQLineEdit::changeEvent(event);
    }
}

void CustomMenuLineEdit::keyPressEvent(QKeyEvent *event)
{
    BlockableQLineEdit::keyPressEvent(event);
}

void CustomMenuLineEdit::setColorScheme(bool darkMode)
{
    menu_->setColorScheme(darkMode);
}

void CustomMenuLineEdit::updateScaling()
{
    menu_->clearItems();
    menu_->clear();
    menu_->initContextMenu();
}

void CustomMenuLineEdit::appendText(const QString &str)
{
    setText(text() + str);
    update();
}

void CustomMenuLineEdit::onMenuTriggered(QAction *action)
{
    QVariant newItem = action->data();

    if (newItem.toInt() == CustomMenuWidget::ACT_UNDO)
    {
        undo();
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_REDO)
    {
        redo();
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_CUT)
    {
        cut();
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_COPY)
    {
        copy();
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_PASTE)
    {
        paste();
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_DELETE)
    {
        insert(""); // deletes selected text
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_SELECT_ALL)
    {
        selectAll();
    }

    emit itemClicked(action->text(), newItem);
}

void CustomMenuLineEdit::updateActionsState()
{
    const auto menuActions = menu_->actions();
    for (QAction *action : menuActions)
    {
        QVariant userValue = action->data();

        bool enable = false;
        switch(userValue.toInt())
        {
        case CustomMenuWidget::ACT_UNDO:
            if (isUndoAvailable()) enable = true;
            break;
        case CustomMenuWidget::ACT_REDO:
            if (isRedoAvailable()) enable = true;
            break;
        case CustomMenuWidget::ACT_CUT:
        case CustomMenuWidget::ACT_COPY:
        case CustomMenuWidget::ACT_DELETE:
            if (hasSelectedText()) enable = true;
            break;
        case CustomMenuWidget::ACT_PASTE:
        {
            const QMimeData *mimeData = QApplication::clipboard()->mimeData();
            if (mimeData->hasText()) enable = true;
            break;
        }
        case CustomMenuWidget::ACT_SELECT_ALL:
            enable = true;
            break;
        }

        action->setEnabled(enable);
    }
}

void CustomMenuLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    updatePositions();
}

void CustomMenuLineEdit::setEchoMode(QLineEdit::EchoMode mode)
{
    echoMode_ = mode;
    updateRevealPassword();
}

void CustomMenuLineEdit::setShowRevealToggle(bool show)
{
    showRevealToggle_ = show;
    updateRevealPassword();
}

void CustomMenuLineEdit::setCustomIcon1(const QString &iconUrl)
{
    WS_ASSERT(icon1_ == nullptr);
    if (icon1_ == nullptr) {
        isCustomIcon1_ = true;
        icon1_ = new IconButtonWidget(iconUrl, this);
        icon1_->show();
        connect(icon1_, &IconButtonWidget::clicked, this, &CustomMenuLineEdit::onIcon1Clicked);
    }
    updatePositions();
}

void CustomMenuLineEdit::setCustomIcon2(const QString &iconUrl)
{
    WS_ASSERT(icon2_ == nullptr);
    if (icon2_ == nullptr) {
        isCustomIcon2_ = true;
        icon2_ = new IconButtonWidget(iconUrl, this);
        icon2_->show();
        connect(icon2_, &IconButtonWidget::clicked, this, &CustomMenuLineEdit::onIcon2Clicked);
    }
    updatePositions();
}

void CustomMenuLineEdit::onIcon1Clicked()
{
    emit icon1Clicked();
}

void CustomMenuLineEdit::onIcon2Clicked()
{
    emit icon2Clicked();
}

void CustomMenuLineEdit::updateRevealPassword()
{
    if (echoMode_ != QLineEdit::Password || !showRevealToggle_) {
        QLineEdit::setEchoMode(QLineEdit::Normal);
        isRevealed_ = false;
        updatePositions();
        return;
    }

    if (icon_ == nullptr) {
        icon_ = new IconButtonWidget("", this);
        icon_->show();
        connect(icon_, &IconButtonWidget::clicked, this, &CustomMenuLineEdit::onRevealClicked);
    }

    if (isRevealed_) {
        QLineEdit::setEchoMode(QLineEdit::Normal);
        icon_->setImage("PASSWORD_HIDE");
    } else {
        QLineEdit::setEchoMode(QLineEdit::Password);
        icon_->setImage("PASSWORD_REVEAL");
    }
    updatePositions();
}

void CustomMenuLineEdit::updatePositions()
{
    const int iconSize = 24*G_SCALE;
    const int iconSpacing = 8*G_SCALE;
    int rightMargin = 4*G_SCALE;
    int xPos = width();

    // Handle icon2 (rightmost)
    if (isCustomIcon2_ && icon2_) {
        xPos -= iconSize + iconSpacing;
        icon2_->setGeometry(xPos, height()/2 - iconSize/2, iconSize, iconSize);
        icon2_->show();
        rightMargin = width() - xPos + 4*G_SCALE;
    } else if (icon2_) {
        icon2_->hide();
    }

    // Handle icon1 (second from right, or rightmost if icon2 is not set)
    if (isCustomIcon1_ && icon1_) {
        xPos -= iconSize + iconSpacing;
        icon1_->setGeometry(xPos, height()/2 - iconSize/2, iconSize, iconSize);
        icon1_->show();
        rightMargin = width() - xPos + 4*G_SCALE;
    } else if (icon1_) {
        icon1_->hide();
    }

    // Handle reveal password icon (uses the old icon_ variable for backward compatibility)
    if ((echoMode_ == QLineEdit::Password && showRevealToggle_) && !isCustomIcon1_ && !isCustomIcon2_) {
        xPos -= iconSize + iconSpacing;
        if (icon_) {
            icon_->setGeometry(xPos, height()/2 - iconSize/2, iconSize, iconSize);
            icon_->show();
            rightMargin = width() - xPos + 4*G_SCALE;
        }
    } else if (icon_ && (echoMode_ != QLineEdit::Password || !showRevealToggle_)) {
        icon_->hide();
    }

    setTextMargins(0, 0, rightMargin, 0);
}

void CustomMenuLineEdit::onRevealClicked()
{
    isRevealed_ = !isRevealed_;
    updateRevealPassword();
}

} // namespace
