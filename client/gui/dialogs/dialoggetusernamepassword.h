#ifndef DIALOGGETUSERNAMEPASSWORD_H
#define DIALOGGETUSERNAMEPASSWORD_H

#include <QDialog>

namespace Ui {
class DialogGetUsernamePassword;
}

class DialogGetUsernamePassword : public QDialog
{
    Q_OBJECT

public:
    explicit DialogGetUsernamePassword(QWidget *parent, bool isRequestUsername);
    ~DialogGetUsernamePassword();

    QString username() { Q_ASSERT(isRequestUsername_); return username_; }
    QString password() { return password_; }
    bool isNeedSave() { return isSave_; }

private slots:
    void onOkClick();
    void onCancelClick();

private:
    Ui::DialogGetUsernamePassword *ui;
    bool isRequestUsername_;
    QString username_;
    QString password_;
    bool isSave_;
};

#endif // DIALOGGETUSERNAMEPASSWORD_H
