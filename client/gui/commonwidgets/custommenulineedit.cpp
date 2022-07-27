#include "custommenulineedit.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QStyle>
#include "graphicresources/fontmanager.h"

// #include <QDebug>

namespace CommonWidgets {

CustomMenuLineEdit::CustomMenuLineEdit(QWidget *parent) : BlockableQLineEdit (parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);

    menu_ = new CustomMenuWidget();
    menu_->setColorScheme(false);
    menu_->initContextMenu();
    connect(menu_, SIGNAL(triggered(QAction*)), SLOT(onMenuTriggered(QAction*)));

}

void CustomMenuLineEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        updateActionsState();
        menu_->exec(QCursor::pos());
    }
    else
    {
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

} // namespace
