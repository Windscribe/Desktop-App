#include "installbutton.h"

#include <QPainter>
#include <QVariantAnimation>

#include "svgresources.h"
#include "languagecontroller.h"
#include "themecontroller.h"

InstallButton::InstallButton(QWidget *parent) : QPushButton(parent), installing_(false)
{
    QString installText = tr("Install");
    QFontMetrics fm(ThemeController::instance().defaultFont(16));
    int textWidth = fm.horizontalAdvance(installText);

    setFixedWidth(textWidth + 68);
    setFixedHeight(44);

    icon_ = new QLabel(this);
    icon_->setPixmap(SvgResources::instance().pixmap(":/resources/INSTALL_ICON.svg"));
    icon_->show();

    spinner_ = new Spinner(this);
    spinner_->hide();

    text_ = new QLabel(this);
    text_->setFont(ThemeController::instance().defaultFont(16));
    text_->setStyleSheet("QLabel { color: #020d1c; }");
    text_->setText(installText);

    if (LanguageController::instance().isRtlLanguage()) {
        icon_->move(14, 14);
        spinner_->setGeometry(14, 14, 16, 16);
        text_->move(46, 12);
        text_->setLayoutDirection(Qt::RightToLeft);
    } else {
        icon_->move(textWidth + 30, 14);
        spinner_->setGeometry(textWidth + 30, 14, 16, 16);
        text_->move(22, 12);
    }

    setStyleSheet("QPushButton { background-color: #55ff8a; border-radius: 22px; }"
                  "QPushButton:hover { background-color: #ffffff; }"
                  "QPushButton:pressed { background-color: #ffffff; }");
}

void InstallButton::setProgress(int progress)
{
    if (!installing_) {
        installing_ = true;
        icon_->hide();
        spinner_->start();
        spinner_->show();
        setStyleSheet("QPushButton { background-color: #ffffff; border-radius: 22px; }");
    }

    if (progress < 100) {
        text_->setText(tr("%1%").arg(progress));

        QFontMetrics fm(ThemeController::instance().defaultFont(16));
        int textWidth = fm.horizontalAdvance(tr("%1%").arg(progress));
        int availableSpace = width() - 68;

        if (LanguageController::instance().isRtlLanguage()) {
            text_->setLayoutDirection(Qt::RightToLeft);
            text_->setGeometry(46 + availableSpace/2 - textWidth/2, 12, textWidth, 20);
        } else {
            text_->setGeometry(22 + availableSpace/2 - textWidth/2, 12, textWidth, 20);
        }
    } else {
        text_->hide();
        setFixedWidth(44);
        spinner_->move(14, 14);
    }
}
