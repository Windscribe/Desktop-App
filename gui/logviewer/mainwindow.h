#pragma once

#include <QVector>
#include <QWidget>
#include "common.h"

class QDragEnterEvent;
class QDropEvent;
class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class QLineEdit;
class QToolButton;
class LogWatcher;
class LogData;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    void initLogData(const QStringList &filenames);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    void openLogFile();
    void saveLogFile();
    void clearLogFile();
    void setHighlight(bool value);
    void setMultiColumn(bool value);
    void setAutoScroll(bool value);
    void setFilterCaseSensitive(bool value);
    void setHideUnmatched(bool value);
    void setFilter(QString filter);
    void applyFilter();
    void gotoPrevMatch();
    void gotoNextMatch();
    void onDataUpdated();

private:
    enum class LogDisplayMode { SINGLE_COLUMN, MULTI_COLUMNS };
    enum class CheckRangeMode { NO, YES, ASK };
    LogDataType chooseLogType(const QString &filename);
    void appendLogFromFile(const QString &filename);
    void setOverrideCursorIfNeeded();
    void restoreOverrideCursor();
    bool checkFilter(LogDataType type, const QString &text) const;
    void ensureLineNumberVisible(int lineNumber);
    void updatePlaceholderText();
    void updateScale();
    void updateHighlight(bool update_matching_lines_only = false);
    void updateDisplay();
    void updateScroll();
    void updateMatchLabel();

    QPlainTextEdit *timeEdit_;
    QPlainTextEdit *textEdit_[NUM_LOG_TYPES];
    QLabel *timeLabel_;
    QLabel *textLabel_[NUM_LOG_TYPES];
    QWidget *textWidget_[NUM_LOG_TYPES];
    QPushButton *btnOpenLog_;
    QPushButton *btnSaveLog_;
    QPushButton *btnClearLog_;
    QCheckBox *cbMultiColumn_;
    QCheckBox *cbHighlight_;
    QCheckBox *cbAutoScroll_;
    QCheckBox *cbFilterCI_;
    QCheckBox *cbHideUnmatched_;
    QLineEdit *leFilter_;
    QLabel *matchLabel_;
    QToolButton *btnNavigate_[2];
    QTimer *filterTimer_;

    double dpiScale_;
    bool logAutoScrollMode_;
    bool logHightlightMode_;
    bool overrideCursorSet_;
    LogDisplayMode logDisplayMode_;
    std::unique_ptr<LogWatcher> logWatcher_;
    std::unique_ptr<LogData> logData_;
    CheckRangeMode checkRangeOnAppend_;
    QString openFilePath_;
    QString saveFilePath_;
    QString currentFilter_;
    bool isFilterCI_;
    bool isHideUnmatched_;
    int textVisibilityMask_;
    int currentFilterMatch_;
    int previousFilterMatch_;
    QVector<int> filterMatches_;
};
