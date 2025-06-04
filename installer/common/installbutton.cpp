#include "installbutton.h"

#include <QPainter>
#include <QVariantAnimation>

#include "svgresources.h"
#include "languagecontroller.h"
#include "themecontroller.h"

InstallButton::InstallButton(QWidget *parent) : QPushButton(parent)
{
    QString installText = tr("Install");
    QFontMetrics fm(ThemeController::instance().defaultFont(15, QFont::Normal));
    int textWidth = fm.horizontalAdvance(installText);

    setFixedWidth(textWidth + 52);
    setFixedHeight(38);

    icon_ = new QLabel(this);
    icon_->setPixmap(SvgResources::instance().pixmap(":/resources/INSTALL_ICON.svg"));
    icon_->show();

    text_ = new QLabel(this);
    text_->setFont(ThemeController::instance().defaultFont(15, QFont::Normal));
    text_->setStyleSheet("QLabel { color: #020d1c; }");
    text_->setText(installText);

    if (LanguageController::instance().isRtlLanguage()) {
        icon_->move(8, 10);
        text_->move(42, 8);
        text_->setLayoutDirection(Qt::RightToLeft);
    } else {
        icon_->move(textWidth + 24, 10);
        text_->move(18, 8);
    }

    setStyleSheet("QPushButton { background-color: #ffffff; border-radius: 19px; }"
                  "QPushButton:hover { background-color: #55ff8a; }"
                  "QPushButton:pressed { background-color: #55ff8a; }");
}
