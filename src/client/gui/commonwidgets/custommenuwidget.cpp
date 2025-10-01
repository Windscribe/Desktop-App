#include "custommenuwidget.h"

#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

CustomMenuWidget::CustomMenuWidget() : QMenu(), fontDescr_(12, QFont::Normal), is_dark_mode_(false)
{
}

void CustomMenuWidget::clearItems()
{
    items_.clear();
}

void CustomMenuWidget::addItem(const QString &caption, const QVariant &userValue)
{
    items_ << PreferencesWindow::ComboBoxItemDescr(caption, userValue);
    QAction *action = addAction(caption);
    action->setFont(FontManager::instance().getFont(fontDescr_));

    // Add key-bindings to display
    if (userValue.toInt() == ACT_UNDO)            { action->setShortcut(QKeySequence::Undo);       }
    else if (userValue.toInt() == ACT_REDO)       { action->setShortcut(QKeySequence::Redo);       }
    else if (userValue.toInt() == ACT_COPY)       { action->setShortcut(QKeySequence::Copy);       }
    else if (userValue.toInt() == ACT_CUT)        { action->setShortcut(QKeySequence::Cut);        }
    else if (userValue.toInt() == ACT_PASTE)      { action->setShortcut(QKeySequence::Paste);      }
    else if (userValue.toInt() == ACT_DELETE)     { action->setShortcut(QKeySequence::Delete);     }
    else if (userValue.toInt() == ACT_SELECT_ALL) { action->setShortcut(QKeySequence::SelectAll);  }

    action->setData(userValue);
}

QAction * CustomMenuWidget::action(CustomMenuWidget::DefaultAction act)
{
    QAction *found = nullptr;
    const auto actionList = actions();
    for (QAction *action : actionList)
    {
        QVariant userValue = action->data();

        if (userValue.toInt() == act)
        {
            found = action;
            break;
        }
    }

    return found;
}

void CustomMenuWidget::setColorScheme(bool darkMode)
{
    is_dark_mode_ = darkMode;
    updateStyleSheet();
}

void CustomMenuWidget::initContextMenu()
{
    // It is important to set the QMenu font, because Qt uses this font (and not the font of an
    // action) to measure size of the action's shortcut.
    setFont(FontManager::instance().getFont(fontDescr_));
    updateStyleSheet();

    addItem(tr("Undo"), ACT_UNDO);
    addItem(tr("Redo"), ACT_REDO);

    addSeparator();
    addItem(tr("Cut"), ACT_CUT);
    addItem(tr("Copy"), ACT_COPY);
    addItem(tr("Paste"), ACT_PASTE);
    addItem(tr("Delete"), ACT_DELETE);

    addSeparator();
    addItem(tr("Select All"), ACT_SELECT_ALL);
}

void CustomMenuWidget::updateStyleSheet()
{
    if (is_dark_mode_)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        setStyleSheet("QMenu { background: rgba(2,13,28, 70%); } ");
        setStyleSheet(styleSheet().append("QMenu::separator {  background-color: rgb(125,125,125) }"));
        setStyleSheet(styleSheet().append("QMenu::item:selected { color: rgb(2,13,28); background-color: rgba(255,255,255, 60%) }"));
        setStyleSheet(styleSheet().append("QMenu::item:!selected { color: white }"));
        setStyleSheet(styleSheet().append("QMenu::item:!selected:!enabled { color: rgb(180,180,180) }"));
    }
    else
    {
        setStyleSheet("QMenu { border: 0px } ");
        setStyleSheet(styleSheet().append("QMenu { color: rgb(2,13,28) }"));
        setStyleSheet(styleSheet().append("QMenu { background-color: rgb(255,255,255); } "));
        setStyleSheet(styleSheet().append("QMenu::separator { background-color: rgb(175,175,175)} "));
        setStyleSheet(styleSheet().append("QMenu::item:selected { background-color: rgb(200,200,200);}"));
        setStyleSheet(styleSheet().append("QMenu::item:!selected { background-color: rgb(255,255,255) }"));
        setStyleSheet(styleSheet().append("QMenu::item:!selected:!enabled { color: rgb(180,180,180) }"));
    }

    auto sizes_stylesheet = QString(
        "QMenu { padding-top: %1px; padding-bottom: %1px } "
        "QMenu::separator { margin: %2px; height: 1px } "
        "QMenu::item { padding-left: %3px; padding-right: %3px } ")
        .arg(10 * G_SCALE).arg(3 * G_SCALE).arg(20 * G_SCALE);
    setStyleSheet(styleSheet().append(sizes_stylesheet));
}
