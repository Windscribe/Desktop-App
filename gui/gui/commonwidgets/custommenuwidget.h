#ifndef CUSTOMMENUWIDGET_H
#define CUSTOMMENUWIDGET_H

#include <QMenu>
#include "preferenceswindow/comboboxitem.h"
#include "graphicresources/fontdescr.h"

class CustomMenuWidget : public QMenu
{
public:

    enum DefaultAction { UNDO, REDO, CUT, COPY, PASTE, DELETE, SELECT_ALL };

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

#endif // CUSTOMMENUWIDGET_H
