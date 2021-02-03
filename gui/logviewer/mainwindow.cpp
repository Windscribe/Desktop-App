#include "mainwindow.h"
#include "logdata.h"
#include "logwatcher.h"
#include "texthighlighter.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>


namespace
{
const char *kLogTitles[NUM_LOG_TYPES] = { "GUI", "Engine", "Service" };

QFont GetMonospaceFont(int pointSize)
{
    QFont font("monospace", pointSize);
    if (QFontInfo(font).fixedPitch()) return font;
    font.setStyleHint(QFont::Monospace);
    if (QFontInfo(font).fixedPitch()) return font;
    font.setStyleHint(QFont::TypeWriter);
    if (QFontInfo(font).fixedPitch()) return font;
    font.setFamily("courier");
    if (QFontInfo(font).fixedPitch()) return font;
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}
}  // namespace

MainWindow::MainWindow() : QWidget(), dpiScale_(1.0), logAutoScrollMode_(true),
                           logHightlightMode_(true), logDisplayMode_(LogDisplayMode::SINGLE_COLUMN),
                           logWatcher_(new LogWatcher), logData_(new LogData),
                           checkRangeOnAppend_(CheckRangeMode::NO),
                           openFilePath_(QStandardPaths::writableLocation(
                               QStandardPaths::DataLocation)),
                           isFilterCI_(true), isHideUnmatched_(true)
{
    // Default path for logs.
    openFilePath_.replace(qApp->applicationName(), "Windscribe2");

    // Window-backround palette.
    QPalette window_backround_palette;
    window_backround_palette.setBrush(QPalette::Active, QPalette::Base,
        palette().brush(QPalette::Active, QPalette::Window));
    window_backround_palette.setBrush(QPalette::Inactive, QPalette::Base,
        palette().brush(QPalette::Inactive, QPalette::Window));
    window_backround_palette.setBrush(QPalette::Disabled, QPalette::Base,
        palette().brush(QPalette::Disabled, QPalette::Window));

    // Setup controls.
    btnOpenLog_ = new QPushButton(tr("Open Logs..."), this);
    connect(btnOpenLog_, SIGNAL(clicked()), SLOT(openLogFile()));
    btnSaveLog_ = new QPushButton(tr("Save Logs..."), this);
    connect(btnSaveLog_, SIGNAL(clicked()), SLOT(saveLogFile()));
    btnClearLog_ = new QPushButton(tr("Clear Logs"), this);
    connect(btnClearLog_, SIGNAL(clicked()), SLOT(clearLogFile()));
    cbMultiColumn_ = new QCheckBox(tr("Multi-column"), this);
    cbMultiColumn_->setChecked(logDisplayMode_ == LogDisplayMode::MULTI_COLUMNS);
    cbMultiColumn_->setEnabled(false);
    connect(cbMultiColumn_, SIGNAL(toggled(bool)), SLOT(setMultiColumn(bool)));
    cbHighlight_ = new QCheckBox(tr("Color highlighting"), this);
    cbHighlight_->setChecked(logHightlightMode_);
    connect(cbHighlight_, SIGNAL(toggled(bool)), SLOT(setHighlight(bool)));
    cbAutoScroll_ = new QCheckBox(tr("Auto-scroll"), this);
    cbAutoScroll_->setChecked(logAutoScrollMode_);
    connect(cbAutoScroll_, SIGNAL(toggled(bool)), SLOT(setAutoScroll(bool)));
    cbFilterCI_ = new QCheckBox(tr("CI"), this);
    cbFilterCI_->setToolTip(tr("Use case-insensitive filter"));
    cbFilterCI_->setChecked(isFilterCI_);
    connect(cbFilterCI_, SIGNAL(toggled(bool)), SLOT(setFilterCaseSensitive(bool)));
    cbHideUnmatched_ = new QCheckBox(tr("Hide"), this);
    cbHideUnmatched_->setToolTip(tr("Hide unmatched lines"));
    cbHideUnmatched_->setChecked(isHideUnmatched_);
    connect(cbHideUnmatched_, SIGNAL(toggled(bool)), SLOT(setHideUnmatched(bool)));

    auto *horzLine = new QFrame(this);
    horzLine->setMaximumHeight(3);
    horzLine->setFrameShape(QFrame::HLine);
    horzLine->setFrameShadow(QFrame::Sunken);

    leFilter_ = new QLineEdit(this);
    leFilter_->setMinimumWidth(200 * dpiScale_);
    leFilter_->setPlaceholderText(tr("Enter text filter..."));
    connect(leFilter_, SIGNAL(textEdited(QString)), SLOT(applyFilter(QString)));
    timeLabel_ = new QLabel(tr("Timestamp") + ":", this);
    timeEdit_ = new QPlainTextEdit(this);
    timeEdit_->setReadOnly(true);
    timeEdit_->setUndoRedoEnabled(false);
    timeEdit_->setMaximumWidth(timeEdit_->minimumWidth());
    timeEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    timeEdit_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    timeEdit_->setPalette(window_backround_palette);
    timeEdit_->setAutoFillBackground(true);
    QSplitter *splitter = new QSplitter(this);
    for (int i = 0; i < NUM_LOG_TYPES; ++i) {
        textLabel_[i] = new QLabel(tr(kLogTitles[i]) + ":", this);
        textEdit_[i] = new QPlainTextEdit(this);
        textEdit_[i]->setReadOnly(true);
        textEdit_[i]->setUndoRedoEnabled(false);
        textEdit_[i]->setLineWrapMode(QPlainTextEdit::NoWrap);
        textEdit_[i]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        textEdit_[i]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit_[i]->setPalette(window_backround_palette);
        textEdit_[i]->setAutoFillBackground(true);
        connect(timeEdit_->verticalScrollBar(), SIGNAL(valueChanged(int)),
                textEdit_[i]->verticalScrollBar(), SLOT(setValue(int)));
        connect(textEdit_[i]->verticalScrollBar(), SIGNAL(valueChanged(int)),
                timeEdit_->verticalScrollBar(), SLOT(setValue(int)));
        auto *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->addWidget(textLabel_[i]);
        vlayout->addWidget(textEdit_[i], 1);
        textWidget_[i] = new QWidget;
        textWidget_[i]->setLayout(vlayout);
        textWidget_[i]->setVisible(false);
        splitter->addWidget(textWidget_[i]);
    }

    // Setup layouts.
    auto *toplayout = new QHBoxLayout;
    toplayout->setAlignment(Qt::AlignLeft);
    toplayout->addWidget(btnOpenLog_);
    toplayout->addWidget(btnSaveLog_);
    toplayout->addWidget(btnClearLog_);
    toplayout->addWidget(cbMultiColumn_);
    toplayout->addWidget(cbHighlight_);
    toplayout->addWidget(cbAutoScroll_);
    toplayout->addStretch(1);
    toplayout->addWidget(leFilter_);
    toplayout->addWidget(cbFilterCI_);
    toplayout->addWidget(cbHideUnmatched_);
    auto *loghlayout = new QHBoxLayout;
    auto *logvlayout = new QVBoxLayout;
    logvlayout->addWidget(timeLabel_);
    logvlayout->addWidget(timeEdit_, 1);
    loghlayout->setAlignment(Qt::AlignCenter);
    loghlayout->addLayout(logvlayout);
    loghlayout->addWidget(splitter);
    auto *vlayout = new QVBoxLayout(this);
    vlayout->setAlignment(Qt::AlignCenter);
    vlayout->addLayout(toplayout);
    vlayout->addWidget(horzLine);
    vlayout->addLayout(loghlayout);

    // Make size of dialog to 70% of desktop size.
    const auto *desktopWidget = QApplication::desktop();
    const auto desktopRc = desktopWidget->availableGeometry(this);
    setGeometry(desktopRc.left() + desktopRc.width() * 0.3 / 2,
        desktopRc.top() + desktopRc.height() * 0.3 / 2,
        desktopRc.width() * 0.7,
        desktopRc.height() * 0.7);

    // Setup log watching.
    connect(logWatcher_.get(),
            SIGNAL(logLinesReady(QStringList, LogDataType, quint32, LogRangeCheckType)),
            logData_.get(), SLOT(addLines(QStringList, LogDataType, quint32, LogRangeCheckType)));
    connect(logWatcher_.get(), SIGNAL(logTypeRemoved(LogDataType)),
            logData_.get(), SLOT(clearDataByLogType(LogDataType)));
    connect(logWatcher_.get(), SIGNAL(logIndexRemoved(quint32)),
            logData_.get(), SLOT(clearDataByLogIndex(quint32)));
    connect(logData_.get(), SIGNAL(dataUpdated()), SLOT(onDataUpdated()));

    setAcceptDrops(true);
    updatePlaceholderText();
    updateScale();
    updateDisplay();
}

MainWindow::~MainWindow()
{
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (!mimeData->hasUrls())
        return;

    checkRangeOnAppend_ = CheckRangeMode::ASK;
    const QList<QUrl> filenames = mimeData->urls();
    for (const auto &filename : filenames)
        appendLogFromFile(filename.toLocalFile());
}

void MainWindow::initLogData(const QStringList &filenames)
{
    checkRangeOnAppend_ = CheckRangeMode::NO;
    logWatcher_->removeAll();
    for (auto &filename : filenames)
        appendLogFromFile(filename);
}

void MainWindow::openLogFile()
{
    auto filenames = QFileDialog::getOpenFileNames(
        this, tr("Append Logs"), openFilePath_, tr("All files (*.*)"), nullptr,
        QFileDialog::DontResolveSymlinks);
    if (filenames.empty())
        return;
    openFilePath_ = QFileInfo(filenames[0]).absolutePath();
    checkRangeOnAppend_ = CheckRangeMode::ASK;
    for (const auto &filename : filenames)
        appendLogFromFile(filename);
}

void MainWindow::saveLogFile()
{
    if (saveFilePath_.isEmpty())
        saveFilePath_ = openFilePath_;
    auto filename = QFileDialog::getSaveFileName(
        this, tr("Save Logs"), saveFilePath_, tr("All files (*.*)"), nullptr,
        QFileDialog::DontResolveSymlinks);
    if (filename.isEmpty())
        return;
    saveFilePath_ = QFileInfo(filename).absolutePath();
    if (!logData_->save(filename))
        QMessageBox::warning(this, QString(), tr("Failed to write \"%1\"!").arg(filename));
}

void MainWindow::clearLogFile()
{
    // Ask for a confirmation, of log data is not empty.
    if (logData_->numTypes() >0 &&
        QMessageBox::No == QMessageBox::question(this, QString(),
        tr("Are you sure to clear the logs?"), QMessageBox::Yes | QMessageBox::No))
        return;
    logWatcher_->removeAll();
}

void MainWindow::setHighlight(bool value)
{
    if (logHightlightMode_ == value)
        return;
    logHightlightMode_ = value;
    updateHighlight();
}

void MainWindow::setMultiColumn(bool value)
{
    const LogDisplayMode dm = value ? LogDisplayMode::MULTI_COLUMNS : LogDisplayMode::SINGLE_COLUMN;
    if (logDisplayMode_ == dm)
        return;
    logDisplayMode_ = dm;
    updateDisplay();
}

void MainWindow::setAutoScroll(bool value)
{
    if (logAutoScrollMode_ == value)
        return;
    logAutoScrollMode_ = value;
}

void MainWindow::setFilterCaseSensitive(bool value)
{
    if (isFilterCI_ != value) {
        isFilterCI_ = value;
        updatePlaceholderText();
        updateDisplay();
    }
}

void MainWindow::setHideUnmatched(bool value)
{
    if (isHideUnmatched_ != value) {
        isHideUnmatched_ = value;
        if (!currentFilter_.isEmpty())
            updateDisplay();
    }
}

void MainWindow::applyFilter(QString filter)
{
    if (currentFilter_ != filter) {
        currentFilter_ = filter;
        updatePlaceholderText();
        updateDisplay();
    }
}

void MainWindow::onDataUpdated()
{
    updatePlaceholderText();
    updateDisplay();
    updateScroll();
}

LogDataType MainWindow::chooseLogType(const QString &filename)
{
    // TODO: make a more convenient UI dialog with radio buttons.
    QMessageBox msgBox;
    QPushButton *buttons[NUM_LOG_TYPES] = {};
    msgBox.setText(tr("What is the log type of \"%1\"?").arg(filename));
    for (size_t i = 0; i < NUM_LOG_TYPES; ++i)
        buttons[i] = msgBox.addButton(tr(kLogTitles[i]), QMessageBox::NoRole);
    msgBox.addButton(tr("<< Skip >>"), QMessageBox::NoRole);
    msgBox.exec();
    for (size_t i = 0; i < NUM_LOG_TYPES; ++i) {
        if (msgBox.clickedButton() == buttons[i])
            return static_cast<LogDataType>(i);
    }
    return LOG_TYPE_UNKNOWN;
}

void MainWindow::appendLogFromFile(const QString &filename)
{
    if (filename.isEmpty())
        return;
    auto logType = LogWatcher::detectLogType(filename);
    if (logType == LOG_TYPE_UNKNOWN) {
        logType = chooseLogType(filename);
        if (logType == LOG_TYPE_UNKNOWN)
            return;
    }
    // If a log of type already exists, ask if we want to append or replace. If the user decides
    // to replace, strip existing items if type. If log is mixed, replace means clearing all items.
    const bool kIsMixed = logType == LOG_TYPE_MIXED;
    const bool kNeedsClear = kIsMixed ? (logData_->numTypes() > 0) : logData_->hasType(logType);
    if (kNeedsClear) {
        const QString msg = tr("%1 log is already loaded. Do you want to clear all the lines "
            "of that type before loading a new log? Choosing \"No\" will keep duplicate lines, if "
            "any.").arg(kIsMixed ? tr("Mixed") : tr(kLogTitles[logType]));
        if (QMessageBox::Yes == QMessageBox::question(this, QString(), msg,
            QMessageBox::Yes | QMessageBox::No))
            logWatcher_->remove(logType);
    }
     if (checkRangeOnAppend_ == CheckRangeMode::ASK) {
         if (logData_->numTypes() != 0 &&
             QMessageBox::Yes == QMessageBox::question(this, QString(),
             tr("Do you want to append only those lines that are within the existing time range?"),
             QMessageBox::Yes | QMessageBox::No))
             checkRangeOnAppend_ = CheckRangeMode::YES;
         else
             checkRangeOnAppend_ = CheckRangeMode::NO;
    }
     if (!logWatcher_->add(filename, logType, (checkRangeOnAppend_ == CheckRangeMode::YES)
         ? LogRangeCheckType::MIN_TO_MAX : LogRangeCheckType::NONE)) {
         QMessageBox::warning(this, QString(), tr("Failed to read \"%1\"!").arg(filename));
     }
}

bool MainWindow::checkFilter(LogDataType type, const QString &text) const
{
    if (type == LOG_TYPE_AUX || currentFilter_.isEmpty())
        return true;
    return text.contains(currentFilter_, isFilterCI_ ? Qt::CaseInsensitive : Qt::CaseSensitive);
}

void MainWindow::updatePlaceholderText()
{
    if (!logData_->numTypes() || currentFilter_.isEmpty())
        textEdit_[0]->setPlaceholderText(tr("Logs are empty"));
    else
        textEdit_[0]->setPlaceholderText(tr("No lines matching \"%1\"")
            .arg(isFilterCI_ ? currentFilter_.toLower() : currentFilter_));
}

void MainWindow::updateScale()
{
    // Get current DPI scale.
#ifdef Q_OS_WIN
    const double kLowestDPI = 96.0;
    const QScreen *screen = window()->windowHandle()
        ? window()->windowHandle()->screen() : qApp->primaryScreen();
    dpiScale_ = (double)screen->logicalDotsPerInch() / kLowestDPI;
#else
    dpiScale_ = 1.0;
#endif
    // Using fixed-pitch fonts.
    const auto textFont{ GetMonospaceFont(font().pointSize()) };
    const int timeEditTextWidth = QFontMetrics(textFont).width("00.00.00 00:00:00:000");

    timeEdit_->setFont(textFont);
    timeEdit_->setMinimumWidth(25 * dpiScale_ + timeEditTextWidth);
    for (int i = 0; i < NUM_LOG_TYPES; ++i)
        textEdit_[i]->setFont(textFont);

    btnOpenLog_->setFont(font());
    btnClearLog_->setFont(font());
}

void MainWindow::updateHighlight()
{
    const bool kIsMulti = logDisplayMode_ == LogDisplayMode::MULTI_COLUMNS
        && logData_->numTypes() > 1;

    for (int i = 0; i < NUM_LOG_TYPES; ++i) {
        if (!textWidget_[i]->isVisible())
            continue;
        TextHighlighter::processDocument(textEdit_[i]->document(), logHightlightMode_,
            kIsMulti ? static_cast<LogDataType>(i) : LOG_TYPE_MIXED);
    }
}

void MainWindow::updateDisplay()
{
    const bool kIsMulti = logDisplayMode_ == LogDisplayMode::MULTI_COLUMNS
        && logData_->numTypes() > 1;
 
    const int scrollPos = timeEdit_->verticalScrollBar()->value();
    const auto &lines = logData_->data();
    const auto global_info = logData_->getDataSizeForType(LOG_TYPE_MIXED);
    const bool kHasFilter = !currentFilter_.isEmpty();
    QString texts[NUM_LOG_TYPES];
    int nonAuxLineCount[NUM_LOG_TYPES] = {};
    int firstFilterMatchLine = -1;

    QString time_string;
    time_string.reserve(global_info.first * 22); // Estimated timestamp size.

    int vismask = 0, oldvismask = 0;
    for (int i = 0; i < NUM_LOG_TYPES; ++i)
        if (textWidget_[i]->isVisible())
            oldvismask |= (1 << i);

    if (kIsMulti) {
        QString timestamp;
        int timestamp_lines[NUM_LOG_TYPES] = {};
        auto trim_lines = [&](const QString &timestamp) {
            const int add =
                qMax(qMax(timestamp_lines[0], timestamp_lines[1]), timestamp_lines[2]);
            for (int i = 0; i < NUM_LOG_TYPES; ++i) {
                const int c = add - timestamp_lines[i];
                if (c > 0)
                    texts[i].append(QString().fill('\n', c));
            }
            for (int i = 1; i < add; ++i)
                time_string.append(timestamp + "\n");
        };
        for (int i = 0; i < NUM_LOG_TYPES; ++i) {
            const auto local_info = logData_->getDataSizeForType(static_cast<LogDataType>(i));
            texts[i].reserve(local_info.second + local_info.first * 6  // Estimated line sizes.
                             + (global_info.first - local_info.first));
        }
        int counter = 0;
        for (auto it = lines.constBegin(); it != lines.constEnd(); ++it) {
            bool is_filter_match = false;
            if (kHasFilter) {
                is_filter_match = checkFilter(it->type, it->text);
                if (!is_filter_match && isHideUnmatched_)
                    continue;
                if (is_filter_match && firstFilterMatchLine < 0)
                    firstFilterMatchLine = counter;
                ++counter;
            }
            if (it->type == LOG_TYPE_AUX || timestamp != it->timestamp) {
                if (!timestamp.isEmpty()) {
                    trim_lines(timestamp);
                    memset(timestamp_lines, 0, sizeof(timestamp_lines));
                }
                time_string.append(it->timestamp + "\n");
                timestamp = it->timestamp;
            }
            if (it->type == LOG_TYPE_AUX) {
                for (int i = 0; i < NUM_LOG_TYPES; ++i)
                    texts[i].append(QString("%1\n").arg(it->text));
            } else {
                ++timestamp_lines[it->type];
                ++nonAuxLineCount[it->type];
                texts[it->type].append(QString("%1[%2] %3\n")
                    .arg(is_filter_match && !isHideUnmatched_ ? "*" : ">", it->label, it->text));
            }
        }
        trim_lines(timestamp);
        textLabel_[0]->setText(tr(kLogTitles[0]) + ":");
    } else {
        const char *kTypeMarker[] = { "G", "E", "S" };
        texts[0].reserve(global_info.second + global_info.first * 6); // Estimated line sizes.
        int counter = 0;
        for (auto it = lines.constBegin(); it != lines.constEnd(); ++it) {
            bool is_filter_match = false;
            if (kHasFilter) {
                is_filter_match = checkFilter(it->type, it->text);
                if (!is_filter_match && isHideUnmatched_)
                    continue;
                if (is_filter_match && firstFilterMatchLine < 0)
                    firstFilterMatchLine = counter;
                ++counter;
            }
            time_string.append(it->timestamp + "\n");
            if (it->type == LOG_TYPE_AUX) {
                texts[0].append(QString("%1\n").arg(it->text));
                continue;
            }
            ++nonAuxLineCount[0];
            texts[0].append(QString("%1%2[%3] %4\n").arg(kTypeMarker[it->type],
                is_filter_match && !isHideUnmatched_ ? "*" : ">", it->label, it->text));
        }
        textLabel_[0]->setText(tr("Combined Logs") + ":");
    }
    timeEdit_->setPlainText(time_string);

    for (int i = 0; i < NUM_LOG_TYPES; ++i) {
        const bool is_empty = texts[i].trimmed().isEmpty();
        if (!is_empty && nonAuxLineCount[i]) {
            textEdit_[i]->setPlainText(texts[i]);
            textWidget_[i]->setVisible(true);
            vismask |= (1 << i);
        } else {
            textEdit_[i]->clear();
            textWidget_[i]->setVisible(false);
        }
    }

    cbMultiColumn_->setEnabled(logData_->numTypes() > 1);
    if (!vismask) {
        timeEdit_->clear();
        textWidget_[0]->setVisible(true);
    } else {
        timeEdit_->verticalScrollBar()->setValue(scrollPos);
    }

    if (!isHideUnmatched_ && firstFilterMatchLine >= 0) {
        const QTextCursor timeCursor(
            timeEdit_->document()->findBlockByLineNumber(firstFilterMatchLine));
        timeEdit_->setTextCursor(timeCursor);
        for (int i = 0; i < NUM_LOG_TYPES; ++i) {
            if (vismask & (1 << i)) {
                const QTextCursor textCursor(
                    textEdit_[i]->document()->findBlockByLineNumber(firstFilterMatchLine));
                textEdit_[i]->setTextCursor(textCursor);
            }
        }
    }

    if (logHightlightMode_) {
        if (vismask == oldvismask)
            updateHighlight();
        else
            QTimer::singleShot(0, [&]() { updateHighlight(); });
    }
}

void MainWindow::updateScroll()
{
    if (logAutoScrollMode_) {
        QTimer::singleShot(0, [&]() {
            timeEdit_->verticalScrollBar()->setValue(
                timeEdit_->verticalScrollBar()->maximum());
        });
    }
}
