#ifndef MULTILINE_MESSAGE_LOGGER_H
#define MULTILINE_MESSAGE_LOGGER_H

#include <QDebug>
#include <QLoggingCategory>

// MultilineMessageLogger: helper class to enable printing of each stream argument on a new line.
// String lists are printed line by line; strings are split into lists by a newline (\n) character.
template<typename OutputHelper>
class MultilineMessageLogger
{
    Q_DISABLE_COPY(MultilineMessageLogger)

public:
    Q_DECL_CONSTEXPR explicit MultilineMessageLogger(const QMessageLogger &logger)
        : logger_(logger) {}

    template<typename U>
    const MultilineMessageLogger &operator<<(U value) const
    {
        OutputHelper::output(logger_) << value;
        return *this;
    }
    const MultilineMessageLogger &operator<<(const QStringList &list) const
    {
        for (const auto &line : list)
            OutputHelper::output(logger_) << line;
        return *this;
    }
    const MultilineMessageLogger &operator<<(const QString &text) const
    {
        return operator<<(text.split("\n", Qt::SkipEmptyParts));
    }
    const MultilineMessageLogger &operator<<(const QByteArray &text) const
    {
        return operator<<(QString(text).split("\n", Qt::SkipEmptyParts));
    }

private:
    const QMessageLogger &logger_;
};

// Output helper classes define which output stream to use (debug, info, warning, etc).
struct MultilineMessageLoggerOutputDebug
{
    static QDebug output(const QMessageLogger &logger) { return logger.debug(); }
};
struct MultilineMessageLoggerOutputInfo
{
    static QDebug output(const QMessageLogger &logger) { return logger.info(); }
};
struct MultilineMessageLoggerOutputWarning
{
    static QDebug output(const QMessageLogger &logger) { return logger.warning(); }
};
struct MultilineMessageLoggerOutputCritical
{
    static QDebug output(const QMessageLogger &logger) { return logger.critical(); }
};

#if !defined(QT_NO_DEBUG_OUTPUT)
#define qCDebugMultiline(category, ...) \
    for (bool qt_category_enabled = category().isDebugEnabled(); qt_category_enabled; qt_category_enabled = false) \
        MultilineMessageLogger<MultilineMessageLoggerOutputDebug>(QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()))
#else
#define qCDebugMultiline(category, ...) QT_NO_QDEBUG_MACRO()
#endif

#if !defined(QT_NO_INFO_OUTPUT)
#  define qCInfoMultiline(category, ...) \
    for (bool qt_category_enabled = category().isInfoEnabled(); qt_category_enabled; qt_category_enabled = false) \
        MultilineMessageLogger<MultilineMessageLoggerOutputInfo>(QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()))
#else
#define qCInfoMultiline(category, ...) QT_NO_QDEBUG_MACRO()
#endif

#if !defined(QT_NO_WARNING_OUTPUT)
#  define qCWarningMultiline(category, ...) \
    for (bool qt_category_enabled = category().isWarningEnabled(); qt_category_enabled; qt_category_enabled = false) \
        MultilineMessageLogger<MultilineMessageLoggerOutputWarning>(QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()))
#else
#define qCWarningMultiline(category, ...) QT_NO_QDEBUG_MACRO()
#endif

#define qCCriticalMultiline(category, ...) \
    for (bool qt_category_enabled = category().isCriticalEnabled(); qt_category_enabled; qt_category_enabled = false) \
        MultilineMessageLogger<MultilineMessageLoggerOutputCritical>(QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()))

#endif  // MULTILINE_MESSAGE_LOGGER_H
