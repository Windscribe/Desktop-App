#ifndef LOGVIEWERWINDOW_H
#define LOGVIEWERWINDOW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "dpiscaleawarewidget.h"

namespace LogViewer {

class LogViewerWindow : public DPIScaleAwareWidget
{
    Q_OBJECT
public:
    explicit LogViewerWindow(QWidget *parent = nullptr);
    ~LogViewerWindow();

    void setLog(const QString &log);

signals:
    void closeClick();

protected:
    void updateScaling() override;

private:
    QPlainTextEdit *textEdit_;
    QVBoxLayout * layout_;
};

}

#endif // LOGVIEWERWINDOW_H
