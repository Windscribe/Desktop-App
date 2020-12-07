#include "logviewerwindow.h"

#include <QTextStream>
#include <QIcon>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace LogViewer {


LogViewerWindow::LogViewerWindow(QWidget *parent) : DPIScaleAwareWidget(parent)
{
    setWindowFlag(Qt::Dialog);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);


    // todo change SVG to ICO file.
    setWindowIcon(ImageResourcesSvg::instance().getIndependentPixmap("BADGE_BLACK_ICON")->getScaledIcon());

    setWindowTitle("Windscribe Log");

    textEdit_ = new QPlainTextEdit();
    textEdit_->setReadOnly(true);

    layout_ = new QVBoxLayout(this);
    layout_->setAlignment(Qt::AlignCenter);
    layout_->addWidget(textEdit_, Qt::AlignCenter);

    // make size of dialog to 70% of desktop size
    QDesktopWidget *desktopWidget = QApplication::desktop();
    QRect desktopRc = desktopWidget->availableGeometry(parent);
    setGeometry(desktopRc.left() + desktopRc.width() * 0.3 / 2,
                desktopRc.top() + desktopRc.height() * 0.3 / 2,
                desktopRc.width() * 0.7,
                desktopRc.height() * 0.7);

    updateScaling();
}

LogViewerWindow::~LogViewerWindow()
{
}

void LogViewerWindow::setLog(const QString &log)
{
    textEdit_->setPlainText(log);
}

void LogViewerWindow::updateScaling()
{
    textEdit_->setFont(*FontManager::instance().getFontWithCustomScale(currentScale(), 12, false));
}

} //namespace LogViewer
