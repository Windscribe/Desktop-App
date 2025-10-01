#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include "dpiscaleawarewidget.h"

class AdvancedParametersDialog : public DPIScaleAwareWidget
{
    Q_OBJECT
public:
    explicit AdvancedParametersDialog(QWidget *parent = nullptr);
    ~AdvancedParametersDialog();

    void setAdvancedParameters(const QString &text);
    const QString advancedParametersText();

signals:
    void okClick();
    void cancelClick();

private slots:
    void onClearClicked();
    void onOkClicked();
    void onCancelClicked();

protected:
    void updateScaling() override;

private:
    QPlainTextEdit *textEdit_;
    QVBoxLayout *layout_;

    QPushButton *clearButton_;
    QPushButton *okButton_;
    QPushButton *cancelButton_;
    QSpacerItem *hSpacer_;
};
