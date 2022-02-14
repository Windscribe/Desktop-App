#include "advancedparametersdialog.h"

#include <QIcon>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"

#include <QDebug>

const int UNSCALED_SPACER_WIDTH = 40;

AdvancedParametersDialog::AdvancedParametersDialog(QWidget *parent) : DPIScaleAwareWidget(parent)
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
    hSpacer_ = new QSpacerItem(UNSCALED_SPACER_WIDTH * currentScale(),1, QSizePolicy::Expanding);

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

#ifdef Q_OS_WIN
    setGeometry(parent->geometry().x() + 22 * currentScale(),
                parent->geometry().y() + 65 * currentScale(),
                350 * currentScale(), 350 * currentScale());
#endif
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
    textEdit_->setFont(*FontManager::instance().getFontWithCustomScale(currentScale(), 12, false));
    // Update viewport font as well, for placeholder text.
    textEdit_->viewport()->setFont(
        *FontManager::instance().getFontWithCustomScale(currentScale(), 12, false));

    hSpacer_->changeSize(UNSCALED_SPACER_WIDTH * currentScale(), 1, QSizePolicy::Expanding);
    clearButton_->setFixedWidth(40 * currentScale());
    okButton_->setFixedWidth(30 * currentScale());
    cancelButton_->setFixedWidth(50 * currentScale());
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
