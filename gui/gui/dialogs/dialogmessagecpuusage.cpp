#include "dialogmessagecpuusage.h"
#include "ui_dialogmessagecpuusage.h"

#include <QDesktopServices>
#include <QPainter>
#include <QUrl>
#include <QStyle>
#include "utils/globalconstants.h"

extern double g_ui_scale;

DialogMessageCpuUsage::DialogMessageCpuUsage(QWidget *parent, const QString &caption) :
    QDialog(parent),
    ui(new Ui::DialogMessageCpuUsage),
    retCode_(RET_NO),
    isIgnoreWarnings_(false)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->lblCaption->setText(caption);

    QString strTransl = tr("Learn More...");
    QString str = "<html><head/><body><p><a href=\"" + strTransl + "\"><span style=\" text-decoration: underline; color:#0000ff;\">" + strTransl + "</span></a></p></body></html>";
    ui->lblLearnMore->setText(str);

    connect(ui->btnYes, SIGNAL(clicked(bool)), SLOT(onYesClicked()));
    connect(ui->btnNo, SIGNAL(clicked(bool)), SLOT(onNoClicked()));
    connect(ui->lblLearnMore, SIGNAL(linkActivated(QString)), SLOT(onHelpClicked()));

    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);
    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    icon_ = icon.pixmap(iconSize, iconSize);

    ui->lblIcon->setMinimumWidth(icon_.width());
    ui->lblIcon->setMaximumWidth(icon_.width());

    ui->lblIcon->installEventFilter(this);
    adjustSize();
}

DialogMessageCpuUsage::~DialogMessageCpuUsage()
{
    delete ui;
}

bool DialogMessageCpuUsage::eventFilter(QObject *watched, QEvent *event)
{
    if (ui->lblIcon == watched && event->type() == QEvent::Paint)
    {
        QPainter painter(ui->lblIcon);
        painter.drawPixmap(0, 0, icon_);
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void DialogMessageCpuUsage::onYesClicked()
{
    retCode_ = RET_YES;
    isIgnoreWarnings_ = ui->cbIgnoreWarnings->isChecked();
    accept();
}

void DialogMessageCpuUsage::onNoClicked()
{
    retCode_ = RET_NO;
    isIgnoreWarnings_ = ui->cbIgnoreWarnings->isChecked();
    accept();
}

void DialogMessageCpuUsage::onHelpClicked()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/support/article/20/tcp-socket-termination").arg(GlobalConstants::instance().serverUrl())));
}
