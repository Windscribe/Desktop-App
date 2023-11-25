#include "dialoggetusernamepassword.h"
#include "ui_dialoggetusernamepassword.h"

DialogGetUsernamePassword::DialogGetUsernamePassword(QWidget *parent, bool isRequestUsername) :
    QDialog(parent),
    ui(new Ui::DialogGetUsernamePassword),
    isRequestUsername_(isRequestUsername)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    this->setWindowTitle(tr("Enter connection credentials"));

    connect(ui->btnOk, &QPushButton::clicked, this, &DialogGetUsernamePassword::onOkClick);
    connect(ui->btnCancel, &QPushButton::clicked, this, &DialogGetUsernamePassword::onCancelClick);
    ui->cbSave->setChecked(true);

    if (!isRequestUsername)
    {
        ui->lblUsername->hide();
        ui->edUsername->hide();
    }
#ifdef Q_OS_WIN
    setMinimumWidth(300);
    setMinimumHeight(130);
#else
    adjustSize();
#endif
}

DialogGetUsernamePassword::~DialogGetUsernamePassword()
{
    delete ui;
}

void DialogGetUsernamePassword::onOkClick()
{
    if (isRequestUsername_ && ui->edUsername->text().isEmpty())
    {
        ui->lblMsg->setText(tr("Please enter the username"));
    }
    else if (ui->edPassword->text().isEmpty())
    {
        ui->lblMsg->setText(tr("Please enter the password"));
    }
    else
    {
        isSave_ = ui->cbSave->isChecked();
        username_ = ui->edUsername->text();
        password_ = ui->edPassword->text();
        accept();
    }
}

void DialogGetUsernamePassword::onCancelClick()
{
    reject();
}
