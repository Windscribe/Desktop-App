#include "progressdisplay.h"

#include "themecontroller.h"

ProgressDisplay::ProgressDisplay(QWidget *parent) : QWidget(parent)
{
    setFixedSize(72, 72);

    progress_ = new QLabel(this);
    progress_->setFont(ThemeController::instance().defaultFont(15, QFont::Medium));
    progress_->setFixedSize(72, 72);
    progress_->move(0, 0);
    progress_->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    progress_->setStyleSheet("QLabel { color: #ffffff; }");
    progress_->show();

    spinner_ = new Spinner(this);
    spinner_->setFixedSize(72, 72);
    spinner_->move(0, 0);
    spinner_->setStyleSheet("QLabel { background-color: #ffffff; }");
    spinner_->stackUnder(progress_);
    spinner_->show();
}

void ProgressDisplay::setProgress(int progress)
{
    spinner_->start();
    if (progress >= 100) {
        progress_->setText("");
        progress_->hide();
    } else {
        progress_->setText(tr("%1%").arg(progress));
    }
}
