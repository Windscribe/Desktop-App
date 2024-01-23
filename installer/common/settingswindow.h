#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QVariantAnimation>
#include <QWidget>

#include "hoverbutton.h"
#include "togglebutton.h"

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    SettingsWindow(QWidget *parent);
    ~SettingsWindow();

    void setDimmedBackground(bool dimmed);
    void setCreateShortcut(bool on);
    void setFactoryReset(bool on);
    void setInstallPath(const QString &path);

signals:
    void minimizeClicked();
    void closeClicked();
    void escapeClicked();
    void browseDirClicked();
    void animationFinished(bool dimmed);
    void factoryResetToggled(bool on);
    void createShortcutToggled(bool on);
    void installPathChanged(const QString &path);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEditingFinished();
    void onAnimValueChanged(const QVariant &value);
    void onAnimFinished();

private:
    QLabel *background_;
    QPixmap backgroundPixmap_;

    QVariantAnimation anim_;
    int animValue_;

    QLineEdit *path_;
    ToggleButton *factoryReset_;
    ToggleButton *createShortcut_;

    QFrame *addDivider(int top);
    QLabel *addLabel(int top, const QString &text);
    ToggleButton *addToggle(int top);
    QLineEdit *addLineEdit(int top, const QString &text);
    HoverButton *addIconButton(int top, const QString &path);
};
