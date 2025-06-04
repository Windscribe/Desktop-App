#pragma once

#include <QLabel>
#include <QWidget>

#include "spinner.h"

class ProgressDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressDisplay(QWidget *parent);

    void setProgress(int progress);

private:
    Spinner *spinner_;
    QLabel *progress_;
};
