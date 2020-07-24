#ifndef CUSTOMMENULINEEDIT_H
#define CUSTOMMENULINEEDIT_H

#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include "loginwindow/blockableqlineedit.h"
#include "preferenceswindow/comboboxitem.h"
#include "commonwidgets/custommenuwidget.h"

class CustomMenuLineEdit : public BlockableQLineEdit
{
    Q_OBJECT
public:
    CustomMenuLineEdit(QWidget *parent = Q_NULLPTR);

    void setColorScheme(bool darkMode);
    void updateScaling();

protected:
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);

signals:
    void itemClicked(QString caption, QVariant value);
    void keyPressed(QKeyEvent *event);

private slots:
    void onMenuTriggered(QAction *action);

private:

    void updateActionsState();
    CustomMenuWidget *menu_;

};

#endif // CUSTOMMENULINEEDIT_H
