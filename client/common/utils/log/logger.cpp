#include "logger.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QString>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "spdlog_utils.h"
#include "paths.h"

namespace log_utils {

bool Logger::install(const QString &logFilePath, bool consoleOutput)
{
    QLoggingCategory::setFilterRules("qt.tlsbackend.ossl=false\nqt.network.ssl=false");
    log_utils::paths::deleteOldUnusedLogs();

#ifdef Q_OS_WIN
    std::wstring path = logFilePath.toStdWString();
#else
    std::string path = logFilePath.toStdString();
#endif

    // Initialize spdlog logger
    try
    {
        if (log_utils::isOldLogFormat(path)) {
            // Use the nothrow version of remove since the removal is not critical and we would
            // like to continue with spdlog creation if the removal does fail.
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
        // Create rotation logger with 2 file with unlimited size
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path, SIZE_MAX, 1, true);
        auto defaultLogger = std::make_shared<spdlog::logger>("default", fileSink);
        spdlog::set_default_logger(defaultLogger);

        // Create the logger without formatting for logging output from libraries such as wsnet, which format logs themselves
        auto rawLogger = std::make_shared<spdlog::logger>("raw", fileSink);
        spdlog::register_logger(rawLogger);

#if defined QT_DEBUG
        // Also output logs to stdout in debug mode
        auto sinkDebug = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        spdlog::default_logger()->sinks().push_back(sinkDebug);
        spdlog::get("raw")->sinks().push_back(sinkDebug);
#endif

        // this will trigger flush on every log message
        spdlog::flush_on(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);

        auto formatter = std::make_unique<log_utils::CustomFormatter>(spdlog::details::make_unique<spdlog::pattern_formatter>("{\"tm\": \"%Y-%m-%d %H:%M:%S.%e\", \"lvl\": \"%^%l%$\", %v}"));
        defaultLogger->set_formatter(std::move(formatter));
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        printf("spdlog init failed: %s\n", ex.what());
        return false;
    }

    prevMessageHandler_ = qInstallMessageHandler(myMessageHandler);
    return true;
}


Logger::Logger()
{
    connectionCategoryDefault_ = std::make_unique<QLoggingCategory>("connection");
}

void Logger::myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &s)
{
    // Skip some of the non-value warnings of the Qt library.
    if (type == QtMsgType::QtWarningMsg) {
        static std::vector<std::string> pointlessStrings = {
            "parseIconEntryInfo(): Failed, OSType doesn't match:",
            "OpenType support missing for"};

        for (const auto &it : pointlessStrings) {
            if (s.contains(it.c_str()))
                return;
        }
    }

#ifdef Q_OS_WIN
    // Uncomment this line if you want to see app output in the debugger.
    //::OutputDebugString(qUtf16Printable(s));
#endif

    std::string escapedMsg = log_utils::escape_string(s.toStdString());
    static const std::string fmt = "\"mod\": \"{}\", \"msg\": \"{}\"";
    if (type == QtDebugMsg)
        spdlog::debug(fmt, context.category, escapedMsg);
    else if (type == QtWarningMsg)
        spdlog::warn(fmt, context.category, escapedMsg);
    else if (type == QtInfoMsg)
        spdlog::info(fmt, context.category, escapedMsg);
    else
        spdlog::error(fmt, context.category, escapedMsg);
}

void Logger::startConnectionMode(const std::string &id)
{
    QMutexLocker lock(&mutex_);
    static std::string connectionModeId;
    connectionModeId = "conn_" + id;
    connectionModeLoggingCategory_.reset(new QLoggingCategory(connectionModeId.c_str()));
    connectionMode_ = true;
}

void Logger::endConnectionMode()
{
    QMutexLocker lock(&mutex_);
    connectionMode_ = false;
}

const QLoggingCategory &Logger::connectionModeLoggingCategory()
{
    QMutexLocker lock(&mutex_);
    if (connectionMode_)
        return *connectionModeLoggingCategory_;
    else
        return *connectionCategoryDefault_;
}

spdlog::logger *Logger::getSpdLogger(const std::string &category)
{
    auto it = spd_loggers_.find(category);
    if (it == spd_loggers_.end()) {
        auto formatter = log_utils::createJsonFormatter();
        auto sinks = spdlog::default_logger()->sinks();
        auto logger = std::make_shared<spdlog::logger>(category, sinks.begin(), sinks.end());
        logger->set_formatter(std::move(formatter));
        spd_loggers_[category] = logger;
        return logger.get();
    } else {
        return it->second.get();
    }
}

}  // namespace log_utils
