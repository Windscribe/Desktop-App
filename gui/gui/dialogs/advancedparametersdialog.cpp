#include "advancedparametersdialog.h"

#include <QIcon>
#include <QGuiApplication>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

#include <QDebug>

const int UNSCALED_SPACER_WIDTH = 40;

AdvancedParametersDialog::AdvancedParametersDialog(QWidget *parent) : QWidget(parent)
{
    setWindowFlag(Qt::Dialog);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);

    setWindowIcon(ImageResourcesSvg::instance().getIndependentPixmap("BADGE_BLACK_ICON")->getScaledIcon());
    setWindowTitle("Advanced Parameters");

    textEdit_ = new QPlainTextEdit();
    textEdit_->setPlaceholderText(tr("Write your parameters here"));

    clearButton_ = new QPushButton(tr("Clear"));
    okButton_ = new QPushButton(tr("Ok"));
    cancelButton_ = new QPushButton(tr("Cancel"));
    hSpacer_ = new QSpacerItem(UNSCALED_SPACER_WIDTH*G_SCALE,1, QSizePolicy::Expanding);

    connect(clearButton_, SIGNAL(clicked()), SLOT(onClearClicked()));
    connect(okButton_, SIGNAL(clicked()), SLOT(onOkClicked()));
    connect(cancelButton_, SIGNAL(clicked()), SLOT(onCancelClicked()));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addSpacerItem(hSpacer_);
    buttonLayout->addWidget(okButton_);
    buttonLayout->addWidget(cancelButton_);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignCenter);
    layout_->addWidget(textEdit_, Qt::AlignCenter);
    layout_->addLayout(buttonLayout);

    curScreenScale_ = G_SCALE;
    setGeometry(parent->geometry().x() + 22*G_SCALE,
                parent->geometry().y() + 65*G_SCALE,
                350 * G_SCALE, 350 * G_SCALE);
    updateScaling();
}

AdvancedParametersDialog::~AdvancedParametersDialog()
{

}

void AdvancedParametersDialog::setAdvancedParameters(const QString &text)
{
    textEdit_->setPlainText(text);
}

const QString AdvancedParametersDialog::advancedParametersText()
{
    return textEdit_->toPlainText();
}

void AdvancedParametersDialog::updateScaling()
{
    textEdit_->setFont(*FontManager::instance().getFontWithCustomScale(curScreenScale_, 12, false));
    hSpacer_->changeSize(UNSCALED_SPACER_WIDTH * curScreenScale_, 1, QSizePolicy::Expanding);
    clearButton_->setFixedWidth(40*curScreenScale_);
    okButton_->setFixedWidth(30*curScreenScale_);
    cancelButton_->setFixedWidth(50*curScreenScale_);
    update();
}

void AdvancedParametersDialog::onClearClicked()
{
    textEdit_->clear();
}

void AdvancedParametersDialog::onOkClicked()
{
    emit okClick();
}

void AdvancedParametersDialog::onCancelClicked()
{
    emit cancelClick();
}

void AdvancedParametersDialog::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    QPoint pt(geometry().x() + geometry().width()/2, geometry().y());
    QScreen *screen = QGuiApplication::screenAt(pt);
    if (screen)
    {
        curScreenScale_ = DpiScaleManager::instance().scaleOfScreen(screen);
    }
    updateScaling();
}
