#pragma once

#include <QLabel>
#include <QPushButton>

#include "spinner.h"

class InstallButton : public QPushButton
{
    Q_OBJECT
public:
    explicit InstallButton(QWidget *parent);

private:
    QLabel *icon_;
    QLabel *text_;
};
