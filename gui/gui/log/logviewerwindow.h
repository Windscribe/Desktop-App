#ifndef LOGVIEWERWINDOW_H
#define LOGVIEWERWINDOW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace LogViewer {

class LogViewerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewerWindow(QWidget *parent = nullptr);
    ~LogViewerWindow();

    void setLog(const QString &log);

signals:
    void closeClick();

private:
    QPlainTextEdit *textEdit_;
    QVBoxLayout * layout_;
};

}

#endif // LOGVIEWERWINDOW_H
