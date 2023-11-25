#pragma once

#include <QLabel>
#include <QPushButton>

#include "spinner.h"

class InstallButton : public QPushButton
{
    Q_OBJECT
public:
    explicit InstallButton(QWidget *parent);

    void setProgress(int progress);

private:
    QLabel *icon_;
    QLabel *text_;
    Spinner *spinner_;

    bool installing_;
};

