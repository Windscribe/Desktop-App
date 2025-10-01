#include "customtexteditwidget.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include "commongraphics/commongraphics.h"
#include <QTextBlock>
#include <QSizePolicy>
#include "dpiscalemanager.h"


CustomTextEditWidget::CustomTextEditWidget(QWidget *parent) : QPlainTextEdit (parent)
    , width_(0)
    , height_(0)
    , viewportHeight_(0)
    , stepSize_(1)
    , pressed_(false)
{
    menu_ = new CustomMenuWidget();
    menu_->setColorScheme(false);
    menu_->initContextMenu();
    connect(menu_, &CustomMenuWidget::triggered, this, &CustomTextEditWidget::onMenuTriggered);

    disableActions();
    updateSelectAllAvailability();

    connect(this, &QPlainTextEdit::undoAvailable, this, &CustomTextEditWidget::onUndoAvailableChanged);
    connect(this, &QPlainTextEdit::copyAvailable, this, &CustomTextEditWidget::onCopyAvailableChanged);
    connect(this, &QPlainTextEdit::redoAvailable, this, &CustomTextEditWidget::onRedoAvailableChanged);

    connect(this, &QPlainTextEdit::textChanged, this, &CustomTextEditWidget::onTextChanged);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CustomTextEditWidget::onCursorPosChanged);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CustomTextEditWidget::onBlockCountChanged);
}

QSize CustomTextEditWidget::sizeHint() const
{
    return QSize(width_ * G_SCALE, height_ * G_SCALE);
}

void CustomTextEditWidget::setWidth(int width)
{
    width_ = width;
    updateGeometry();
}

void CustomTextEditWidget::setViewportHeight(int height)
{
    viewportHeight_ = height;
}

void CustomTextEditWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        pressed_ = true;
    }
    else
    {
        QPlainTextEdit::mousePressEvent(event);
    }
}

void CustomTextEditWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressed_)
    {
        pressed_ = false;

        // update paste availability
        bool enable = false;
        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        if (mimeData->hasText()) enable = true;

        QAction * action = menu_->action(CustomMenuWidget::ACT_PASTE);
        if (action != nullptr)
        {
            action->setEnabled(enable);
        }

        menu_->exec(QCursor::pos());
    }
    else
    {
        QPlainTextEdit::mouseReleaseEvent(event);
    }
}

void CustomTextEditWidget::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange)
    {
        menu_->clearItems();
        menu_->clear();
        menu_->initContextMenu();
    }
    else
    {
        QPlainTextEdit::changeEvent(event);
    }
}

void CustomTextEditWidget::onMenuTriggered(QAction *action)
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
        insertPlainText(""); // deletes selected text
    }
    else if (newItem.toInt() == CustomMenuWidget::ACT_SELECT_ALL)
    {
        selectAll();
    }

    emit itemClicked(action->text(), newItem);
}

void CustomTextEditWidget::onUndoAvailableChanged(bool avail)
{
    QAction * action = menu_->action(CustomMenuWidget::ACT_UNDO);
    if (action != nullptr)
    {
        action->setEnabled(avail);
    }
}

void CustomTextEditWidget::onCopyAvailableChanged(bool avail)
{
    QAction * action = menu_->action(CustomMenuWidget::ACT_COPY);
    if (action != nullptr)
    {
        action->setEnabled(avail);
    }

    action = menu_->action(CustomMenuWidget::ACT_CUT);
    if (action != nullptr)
    {
        action->setEnabled(avail);
    }

    action = menu_->action(CustomMenuWidget::ACT_DELETE);
    if (action != nullptr)
    {
        action->setEnabled(avail);
    }
}

void CustomTextEditWidget::onRedoAvailableChanged(bool avail)
{
    QAction * action = menu_->action(CustomMenuWidget::ACT_REDO);
    if (action != nullptr)
    {
        action->setEnabled(avail);
    }
}

void CustomTextEditWidget::updateSelectAllAvailability()
{
    bool selectAllAvailable = toPlainText() != "";

    QAction *action = menu_->action(CustomMenuWidget::ACT_SELECT_ALL);
    if (action != nullptr)
    {
        action->setEnabled(selectAllAvailable);
    }
}

void CustomTextEditWidget::undoableClear()
{
    selectAll();
    insertPlainText("");
}

int CustomTextEditWidget::stepSize() const
{
    return stepSize_ * G_SCALE;
}

void CustomTextEditWidget::setStepSize(int stepSize)
{
    stepSize_ = stepSize;
}

void CustomTextEditWidget::recalcHeight()
{
    // Qt may not update internal document representation immediately after its contents are
    // changed, e.g. after undo/redo. To get correct line count here, we have to update the
    // document manually.
    document()->adjustSize();

    const int newHeight = qMax<int>( viewportHeight_, ((document()->lineCount() + 1) * lineHeight()) / G_SCALE );


    if (newHeight != height_)
    {
        height_ = newHeight;
        updateGeometry();
        emit heightChanged(newHeight);
    }
}

void CustomTextEditWidget::onTextChanged()
{
    updateSelectAllAvailability();
    recalcHeight();
}

void CustomTextEditWidget::shiftToCursor()
{
    int currentLine = textCursor().blockNumber();

    if (currentLine <= firstVisibleLine())
    {
        int diff = abs(currentLine - firstVisibleLine()) + 1;
        int newPos = QWidget::pos().y() + diff*lineHeight();

        if (currentLine <= 0) newPos = 0; // sometimes doesn't scroll all the way
        emit shiftPos(newPos);
    }
    else if (currentLine >= lastVisibleLine())
    {
        int diff = abs(currentLine - lastVisibleLine()) + 1;
        int newPos = QWidget::pos().y() - diff*lineHeight();

        if (currentLine >= blockCount()-1) newPos -= 20; // sometimes doesn't scroll all the way
        emit shiftPos(newPos);
    }
}

void CustomTextEditWidget::onCursorPosChanged()
{
    shiftToCursor();
}

void CustomTextEditWidget::onBlockCountChanged(int newblockCount)
{
    Q_UNUSED(newblockCount);

    recalcHeight();
    emit recenterWidget();
}

void CustomTextEditWidget::disableActions()
{
    const auto menuActions = menu_->actions();
    for (QAction *action : menuActions)
    {
        action->setEnabled(false);
    }
}

int CustomTextEditWidget::firstVisibleLine()
{
    int y = QWidget::pos().y();

    int result = -(y/lineHeight());
    if (result < 0 ) result = 0;
    return result;
}

int CustomTextEditWidget::lastVisibleLine()
{
    return firstVisibleLine() + (viewportHeight_ * G_SCALE)/(lineHeight());
}

int CustomTextEditWidget::lineHeight()
{
#if 1
    return CommonGraphics::textHeight(font()) + 1;
#else
    int result = 1;
    QTextDocument *doc = document();
    if (doc != nullptr)
    {
        result = doc->firstBlock().blockFormat().lineHeight();

        if (result == 0)
        {
            result = 1;
        }
    }

    return result;
#endif
}

void CustomTextEditWidget::updateScaling()
{
    recalcHeight();
    emit recenterWidget();
    menu_->clearItems();
    menu_->clear();
    menu_->initContextMenu();
}
