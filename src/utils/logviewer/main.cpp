#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>


namespace
{
const int typeIdLogDataType = qRegisterMetaType<LogDataType>("LogDataType");
const int typeIdLogRangeCheckType = qRegisterMetaType<LogRangeCheckType>("LogRangeCheckType");

QStringList parseCommandLine()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(qApp->applicationDisplayName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files",
        QCoreApplication::translate("files", "Log files to open (optional)."), "[files...]");
    parser.process(*qApp);
    return parser.positionalArguments();
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Windscribe");
    app.setApplicationName("WindscribeLogViewer");
    app.setApplicationDisplayName("Windscribe Log Viewer");
    app.setApplicationVersion("1.0");
    app.setStyle("fusion");

    MainWindow w;
    auto logfiles = parseCommandLine();
    if (!logfiles.empty())
        w.initLogData(logfiles);
    w.show();

    return app.exec();
}