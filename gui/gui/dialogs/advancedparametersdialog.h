#ifndef ADVANCEDPARAMETERSDIALOG_H
#define ADVANCEDPARAMETERSDIALOG_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>

class AdvancedParametersDialog : public QWidget
{
    Q_OBJECT
public:
    explicit AdvancedParametersDialog(QWidget *parent = nullptr);
    ~AdvancedParametersDialog();

    void setAdvancedParameters(const QString &text);
    const QString advancedParametersText();

    void updateScaling();

signals:
    void okClick();
    void cancelClick();

private slots:
    void onClearClicked();
    void onOkClicked();
    void onCancelClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QPlainTextEdit *textEdit_;
    QVBoxLayout *layout_;

    QPushButton *clearButton_;
    QPushButton *okButton_;
    QPushButton *cancelButton_;
    QSpacerItem *hSpacer_;

    double curScreenScale_;
};

#endif // ADVANCEDPARAMETERSDIALOG_H
