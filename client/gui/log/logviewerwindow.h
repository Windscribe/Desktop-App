#pragma once

#include <QCheckBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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
    void updateLog();
    void updateColorHighlighting(bool isColorHighlighting);
    void onExportClick();
    void onWordWrapToggled(bool wordWrap);
    void onOsThemeChanged(bool isDarkTheme);

protected:
    void updateScaling() override;

private:
    void highlightBlocks();

    static constexpr bool DEFAULT_COLOR_HIGHLIGHTING = false;

    QPlainTextEdit *textEdit_;
    QVBoxLayout *layout_;
    QCheckBox *cbWordWrap_;
    QCheckBox *cbColorHighlighting_;
    QPushButton *btnExportLog_;
    bool isColorHighlighting_;
    bool isDarkTheme_;
};

}
