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
    icon_(nullptr), echoMode_(QLineEdit::Normal), showRevealToggle_(false), isRevealed_(false)
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
    // qCDebug(LOG_USER) << "CusomMenuLineEdit::keyPressEvent";

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
    if (echoMode_ != QLineEdit::Password || !showRevealToggle_) {
        setTextMargins(0, 0, 4*G_SCALE, 0);
        if (icon_) {
            icon_->hide();
        }
    } else {
        const int iconSize = 24*G_SCALE;
        icon_->setGeometry(width() - iconSize - 8*G_SCALE, height()/2 - iconSize/2, iconSize, iconSize);
        setTextMargins(0, 0, iconSize + 12*G_SCALE, 0);
    }
}

void CustomMenuLineEdit::onRevealClicked()
{
    isRevealed_ = !isRevealed_;
    updateRevealPassword();
}

} // namespace
