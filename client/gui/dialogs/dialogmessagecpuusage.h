#ifndef DIALOGMESSAGECPUUSAGE_H
#define DIALOGMESSAGECPUUSAGE_H

#include <QDialog>
#include <QPixmap>

namespace Ui {
class DialogMessageCpuUsage;
}

class DialogMessageCpuUsage : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMessageCpuUsage(QWidget *parent, const QString &caption);
    ~DialogMessageCpuUsage();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    enum RET_CODE {RET_YES, RET_NO};
    RET_CODE retCode() { return retCode_; }
    bool isIgnoreWarnings() { return isIgnoreWarnings_; }

private slots:
    void onYesClicked();
    void onNoClicked();
    void onHelpClicked();

private:
    Ui::DialogMessageCpuUsage *ui;
    RET_CODE retCode_;
    QPixmap icon_;
    bool isIgnoreWarnings_;
};

#endif // DIALOGMESSAGECPUUSAGE_H
