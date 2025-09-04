#include "logviewerwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "randomcolor.h"
#include "themecontroller.h"
#include "utils/log/mergelog.h"

namespace LogViewer {

LogViewerWindow::LogViewerWindow(QWidget *parent)
    : DPIScaleAwareWidget(parent), isColorHighlighting_(DEFAULT_COLOR_HIGHLIGHTING)
{
    setWindowFlag(Qt::Dialog);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);

    // todo change SVG to ICO file.
    setWindowIcon(ImageResourcesSvg::instance().getIndependentPixmap("BADGE_BLACK_ICON")->getScaledIcon());

    setWindowTitle("Windscribe Log");

    textEdit_ = new QPlainTextEdit(this);
    textEdit_->setReadOnly(true);

    cbWordWrap_ = new QCheckBox(this);
    cbWordWrap_->setText(tr("Word Wrap"));
    cbWordWrap_->setChecked(true);
    connect(cbWordWrap_, &QCheckBox::toggled, this, &LogViewerWindow::onWordWrapToggled);

    cbColorHighlighting_ = new QCheckBox(this);
    cbColorHighlighting_->setText(tr("Colour highlighting"));
    cbColorHighlighting_->setChecked(isColorHighlighting_);
    connect(cbColorHighlighting_, &QCheckBox::toggled, this, &LogViewerWindow::updateColorHighlighting);

    btnExportLog_ = new QPushButton("this");
    btnExportLog_->setText(tr("Export to file..."));
    connect(btnExportLog_, &QPushButton::clicked, this, &LogViewerWindow::onExportClick);

    auto *hLayout = new QHBoxLayout();
    hLayout->setAlignment(Qt::AlignLeft);
    hLayout->addWidget(cbWordWrap_);
    hLayout->addWidget(cbColorHighlighting_);
    hLayout->addWidget(btnExportLog_);
    hLayout->addStretch(1);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignCenter);
    layout_->addLayout(hLayout);
    layout_->addWidget(textEdit_, 1);

    // make size of dialog to 70% of desktop size
    QRect desktopRc = screen()->availableGeometry();
    setGeometry(desktopRc.left() + desktopRc.width() * 0.3 / 2,
                desktopRc.top() + desktopRc.height() * 0.3 / 2,
                desktopRc.width() * 0.7,
                desktopRc.height() * 0.7);

    updateScaling();

    QTimer::singleShot(250, this, [this]() { updateLog(); });

    connect(&ThemeController::instance(), &ThemeController::osThemeChanged, this, &LogViewerWindow::onOsThemeChanged);
    onOsThemeChanged(ThemeController::instance().isOsDarkTheme());
}

LogViewerWindow::~LogViewerWindow()
{
}

void LogViewerWindow::updateLog()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit_->setPlainText(log_utils::MergeLog::mergeLogs());
    highlightBlocks();
    QApplication::restoreOverrideCursor();
}

void LogViewerWindow::updateColorHighlighting(bool isColorHighlighting)
{
    isColorHighlighting_ = isColorHighlighting;
    highlightBlocks();
}

void LogViewerWindow::onExportClick()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save log"), QString(), tr("Text files (*.txt)"));
    if (!fileName.isEmpty())
    {
        QString log = log_utils::MergeLog::mergePrevLogs();
        log += "================================================================================================================================================================================================\n";
        log += "================================================================================================================================================================================================\n";
        log += log_utils::MergeLog::mergeLogs();

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(log.toLocal8Bit());
        }
        else
        {
            QMessageBox::information(this, tr("Export log"), tr("Failed to export log.  Make sure you have the correct permissions."));
        }
    }
}

void LogViewerWindow::updateScaling()
{
    textEdit_->setFont(FontManager::instance().getFontWithCustomScale(currentScale(), 12, QFont::Normal));
}

void LogViewerWindow::highlightBlocks()
{
    const auto *doc = textEdit_->document();
    auto block = doc->begin();
    if (block == doc->end())
        return;

    for (; block != doc->end(); block = block.next()) {
        auto blockFormat = block.blockFormat();
        if (block.text().isEmpty())
            continue;

        QJsonParseError errCode;
        auto doc = QJsonDocument::fromJson(QByteArray(block.text().toStdString().c_str()), &errCode);
        if (errCode.error != QJsonParseError::NoError) {
            continue;
        }
        auto jsonObject = doc.object();
        auto moduleTag = jsonObject.value("mod").toString();
        auto hash = qHash(moduleTag);

        if (isColorHighlighting_) {
            RandomColor color;
            color.setSeed(hash);

            // Get the text color for contrast calculation
            QColor textColor = textEdit_->palette().color(QPalette::Text);
            int textColorRgb = (textColor.red() << 16) | (textColor.green() << 8) | textColor.blue();

            int c = color.generateWithContrast(textColorRgb, RandomColor::RandomHue,
                                               isDarkTheme_ ? RandomColor::Dark : RandomColor::Light);
            auto brush = QBrush(QColor(c));
            blockFormat.setBackground(brush);
        } else {
            blockFormat.setBackground(QBrush());
        }
        QTextCursor(block).setBlockFormat(blockFormat);
    }
}

void LogViewerWindow::onWordWrapToggled(bool wordWrap)
{
    textEdit_->setWordWrapMode(wordWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

void LogViewerWindow::onOsThemeChanged(bool isDarkTheme)
{
    isDarkTheme_ = isDarkTheme;
    // Delay to ensure palette has updated after theme change
    QTimer::singleShot(100, this, [this]() { highlightBlocks(); });
}

} //namespace LogViewer
