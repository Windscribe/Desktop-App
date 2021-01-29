#ifndef LOGDATA_H
#define LOGDATA_H

#include <QDateTime>
#include <QHash>
#include <QMultiMap>
#include <QObject>
#include <QString>
#include "common.h"

struct LogDataEntry
{
    LogDataType type;   // Log entry type (GUI, engine, etc.)
    quint32 logindex;   // Log watcher file index.
    QString timestamp;  // Timestamp in human-readable form.
    QString label;      // Prefix label (e.g. [E 60.44])
    QString text;       // Actual log entry text.
};

class LogData final : public QObject
{
    Q_OBJECT

public:
    using LogDataMutableIterator = QMutableMapIterator<quint64, LogDataEntry>;
    using LogDataStorage = QMultiMap<quint64, LogDataEntry>;

    LogData();
    LogData(const LogData&) = delete;
    LogData &operator=(const LogData&) = delete;

    int numTypes() const { return typeInfo_.size(); }
    bool hasType(LogDataType type) const { return typeInfo_.contains(type); }
    QPair<int, int> getDataSizeForType(LogDataType type) const;
    const LogDataStorage &data() const { return data_; }
    bool save(const QString &filename) const;

signals:
    void dataUpdated();

private slots:
    void addLines(QStringList lines, LogDataType type, quint32 index, LogRangeCheckType rangeCheck);
    void clearDataByLogType(LogDataType type);
    void clearDataByLogIndex(quint32 index);

private:
    void processLine(const QString &line, LogDataType type, quint32 index, int yoffset,
                     QDateTime *range);

    QHash<LogDataType, QPair<int, int>> typeInfo_;
    LogDataStorage data_;
    QDateTime lastDateTime_;
    qint32 timestampCounter_;
};

#endif  // LOGDATA_H
