#include "logger.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QString>

#include <spdlog/sinks/rotating_file_sink.h>

#ifdef Q_OS_WIN
#include <spdlog/sinks/msvc_sink.h>
#else
#include <spdlog/sinks/stdout_sinks.h>
#endif

#include "spdlog_utils.h"
#include "paths.h"

namespace log_utils {


// The custom formatter that allows:
// to output unformatted messages for the logger with "raw" name
// to output messages for the logger with "qt" name for message from Qt loggers
// to output messages for all other loggers with standard json pattern

class CustomFormatter : public spdlog::formatter {
public:

    explicit CustomFormatter(std::unique_ptr<spdlog::formatter> formatter)
    {
        formatter_ = std::move(formatter);
        jsonFormatter_ = log_utils::createJsonFormatter();
    }

    virtual ~CustomFormatter() = default;

    void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest) override
    {
        if (msg.logger_name == "raw") {
            dest.append(msg.payload.data(), msg.payload.data() + msg.payload.size());
        } else if (msg.logger_name == "qt") {
            formatter_->format(msg, dest);
        } else {
            jsonFormatter_->format(msg, dest);
        }
    }

    std::unique_ptr<formatter> clone() const override
    {
        return spdlog::details::make_unique<CustomFormatter>(formatter_->clone());
    }

private:
    std::unique_ptr<spdlog::formatter> formatter_;
    std::unique_ptr<spdlog::formatter> jsonFormatter_;
};


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
        auto defaultLogger = std::make_shared<spdlog::logger>("qt", fileSink);
        spdlog::set_default_logger(defaultLogger);

        // Create the logger without formatting for logging output from libraries such as wsnet, which format logs themselves
        auto rawLogger = std::make_shared<spdlog::logger>("raw", fileSink);
        spdlog::register_logger(rawLogger);

        // Also output logs to debugger/stdout for QA/dev use.
#if defined(Q_OS_WIN)
        auto sinkDebug = std::make_shared<spdlog::sinks::msvc_sink_mt>(false);
        spdlog::default_logger()->sinks().push_back(sinkDebug);
        spdlog::get("raw")->sinks().push_back(sinkDebug);
#elif not defined(CLI_ONLY)
        auto sinkDebug = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        sinkDebug->set_level(spdlog::level::trace); // Required to get any output in the terminal on Linux.
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
    static QStringList pointlessStrings = {
        "parseIconEntryInfo(): Failed, OSType doesn't match:",
        "OpenType support missing for",
        "This plugin does not support setting window opacity",
        "Unable to open monitor interface to"
    };

    if (type == QtMsgType::QtInfoMsg || type == QtMsgType::QtWarningMsg) {
        for (const auto &it : pointlessStrings) {
            if (s.contains(it)) {
                return;
            }
        }
    }

    auto qtLogger = spdlog::get("qt");
    std::string escapedMsg = log_utils::escape_string(s.toStdString());
    static const std::string fmt = "\"mod\": \"{}\", \"msg\": \"{}\"";
    if (type == QtDebugMsg)
        qtLogger->debug(fmt, context.category, escapedMsg);
    else if (type == QtWarningMsg)
        qtLogger->warn(fmt, context.category, escapedMsg);
    else if (type == QtInfoMsg)
        qtLogger->info(fmt, context.category, escapedMsg);
    else
        qtLogger->error(fmt, context.category, escapedMsg);
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
        auto sinks = spdlog::default_logger()->sinks();
        auto logger = std::make_shared<spdlog::logger>(category, sinks.begin(), sinks.end());
        spd_loggers_[category] = logger;
        return logger.get();
    } else {
        return it->second.get();
    }
}

}  // namespace log_utils
