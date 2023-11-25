#pragma once

#include <QLabel>
#include <QWidget>

#include "hoverbutton.h"
#include "installbutton.h"

class InitialWindow : public QWidget
{
    Q_OBJECT

public:
    InitialWindow(QWidget *parent);
    ~InitialWindow();

public slots:
    void setProgress(int progress);

signals:
    void installClicked();
    void minimizeClicked();
    void closeClicked();
    void settingsClicked();

private slots:
    void onEulaClicked();
    void onInstallClicked();

private:
    InstallButton *installButton_;
    HoverButton *settingsButton_;
    HoverButton *eulaButton_;
};
