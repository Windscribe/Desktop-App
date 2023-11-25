#pragma once

#include <QMenu>
#include "preferenceswindow/comboboxitem.h"
#include "graphicresources/fontdescr.h"

class CustomMenuWidget : public QMenu
{
    Q_OBJECT
public:

    enum DefaultAction { ACT_UNDO, ACT_REDO, ACT_CUT, ACT_COPY, ACT_PASTE, ACT_DELETE, ACT_SELECT_ALL };

    CustomMenuWidget();

    void clearItems();
    void addItem(const QString &caption, const QVariant &userValue);

    QAction *action(DefaultAction act);

    void setColorScheme(bool darkMode);

    void initContextMenu();

private:
    void updateStyleSheet();

    QList<PreferencesWindow::ComboBoxItemDescr> items_;

    FontDescr fontDescr_;
    bool is_dark_mode_;
};
