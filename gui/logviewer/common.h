#ifndef COMMON_H
#define COMMON_H

#include <QObject>

enum LogDataType {
    // Generic enumerators (valid both as function arguments, return values, and log entry values).
    LOG_TYPE_GUI,
    LOG_TYPE_ENGINE,
    LOG_TYPE_SERVICE,
    NUM_LOG_TYPES,
    // Auxiliary enumerator (valid as log entry value)
    LOG_TYPE_AUX,
    // Special enumerators (valid only as function arguments and return values).
    LOG_TYPE_UNKNOWN = NUM_LOG_TYPES,
    LOG_TYPE_MIXED,
};

enum class LogRangeCheckType {
    NONE,        // Don't do a range check.
    MIN_TO_MAX,  // Range is between minimum and maximum data timestamps.
    MIN_TO_NOW,  // Range is between minimum data timestamp and current time.
};

Q_DECLARE_METATYPE(LogDataType);
Q_DECLARE_METATYPE(LogRangeCheckType);

#endif  // COMMON_H
