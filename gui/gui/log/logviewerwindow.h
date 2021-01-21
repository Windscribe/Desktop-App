#ifndef LOGVIEWERWINDOW_H
#define LOGVIEWERWINDOW_H

#include <QWidget>
#include <QCheckBox>
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

signals:
    void closeClick();

private slots:
    void updateLog(bool doMergePerLine);
    void updateColorHighlighting(bool isColorHighlighting);

protected:
    void updateScaling() override;

private:
    void highlightBlocks();

    static constexpr bool DEFAULT_MERGE_PER_LINE = true;
    static constexpr bool DEFAULT_COLOR_HIGHLIGHTING = false;

    QPlainTextEdit *textEdit_;
    QVBoxLayout *layout_;
    QCheckBox *cbMergePerLine_;
    QCheckBox *cbColorHighlighting_;
    bool isColorHighlighting_;
};

}

#endif // LOGVIEWERWINDOW_H
