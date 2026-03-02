#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include "loginwindow/blockableqlineedit.h"
#include "preferenceswindow/comboboxitem.h"
#include "commonwidgets/custommenuwidget.h"
#include "commonwidgets/iconbuttonwidget.h"

namespace CommonWidgets {

class CustomMenuLineEdit : public BlockableQLineEdit
{
    Q_OBJECT
public:
    CustomMenuLineEdit(QWidget *parent = nullptr);

    void setColorScheme(bool darkMode);
    void updateScaling();

    void appendText(const QString &str);
    void setEchoMode(QLineEdit::EchoMode mode);
    void setShowRevealToggle(bool show);

    void setCustomIcon1(const QString &iconUrl);
    void setCustomIcon2(const QString &iconUrl);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void itemClicked(QString caption, QVariant value);
    void icon1Clicked();
    void icon2Clicked();

private slots:
    void onMenuTriggered(QAction *action);
    void onRevealClicked();
    void onIcon1Clicked();
    void onIcon2Clicked();

private:
    void updateActionsState();
    void updateRevealPassword();
    void updatePositions();

    CustomMenuWidget *menu_;
    IconButtonWidget *icon_;
    IconButtonWidget *icon1_;
    IconButtonWidget *icon2_;
    QLineEdit::EchoMode echoMode_;
    bool showRevealToggle_;
    bool isRevealed_;
    bool isCustomIcon1_ = false;
    bool isCustomIcon2_ = false;
};

}
