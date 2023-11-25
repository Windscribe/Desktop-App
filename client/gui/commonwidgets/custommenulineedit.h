#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include "loginwindow/blockableqlineedit.h"
#include "preferenceswindow/comboboxitem.h"
#include "commonwidgets/custommenuwidget.h"

namespace CommonWidgets {

class CustomMenuLineEdit : public BlockableQLineEdit
{
    Q_OBJECT
public:
    CustomMenuLineEdit(QWidget *parent = nullptr);

    void setColorScheme(bool darkMode);
    void updateScaling();

    void appendText(const QString &str);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event)          override;
    void keyPressEvent(QKeyEvent *event)     override;

signals:
    void itemClicked(QString caption, QVariant value);

private slots:
    void onMenuTriggered(QAction *action);

private:

    void updateActionsState();
    CustomMenuWidget *menu_;

};

}
