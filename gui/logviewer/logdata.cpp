#include "logdata.h"

#include <QFile>
#include <QTextStream>


namespace
{
bool isYearInDatePresent(const QString &dateline)
{
    const int scan = qMin(7, dateline.length());
    for (int i = 1; i < scan; ++i)
        if (dateline[i] == ' ')
            return false;
    return true;
}

int getLogDataEntryStringSize(const LogDataEntry &entry)
{
    return entry.label.length() + entry.text.length();
}
}  // namespace

LogData::LogData() : timestampCounter_(0)
{
}

QPair<int, int> LogData::getDataSizeForType(LogDataType type) const
{
    Q_ASSERT(type < NUM_LOG_TYPES || type == LOG_TYPE_MIXED);
    if (type < NUM_LOG_TYPES)
        return typeInfo_[type];
    QPair<int, int> result;
    for (auto it = typeInfo_.begin(); it != typeInfo_.end(); ++it) {
        result.first += it.value().first;
        result.second += it.value().second;
    }
    return result;
}

bool LogData::save(const QString &filename) const
{
    QFile qf(filename);
    if (!qf.open(QIODeviceBase::WriteOnly))
        return false;

    const char *kTypeMarker[] = { "G", "E", "S" };
    QTextStream qts(&qf);
    qts.setFieldAlignment(QTextStream::AlignRight);
    for (auto it = data_.constBegin(); it != data_.constEnd(); ++it) {
        const auto timestamp =
            QDateTime::fromMSecsSinceEpoch(it.key() >> 20).toString("ddMMyy hh:mm:ss:zzz");
        if (it->type == LOG_TYPE_AUX) {
            qts << it->text + "\n";
        } else {
            qts << kTypeMarker[it->type] << " [" << timestamp
                << qSetFieldWidth(11) << it->label << qSetFieldWidth(0)
                << "] " << it->text + "\n";
        }
    }
    qf.close();
    return true;
}

void LogData::clearDataByLogType(LogDataType type)
{
    Q_ASSERT(type < NUM_LOG_TYPES || type == LOG_TYPE_MIXED);
    bool updated = false;
    if (type == LOG_TYPE_MIXED) {
        typeInfo_.clear();
        data_.clear();
        timestampCounter_ = 0;
        updated = true;
    } else {
        typeInfo_.remove(type);
        LogDataMutableIterator it(data_);
        while (it.hasNext()) {
            it.next();
            if (it.value().type == type) {
                it.remove();
                updated = true;
            }
        }
    }
    if (updated)
        emit dataUpdated();
}

void LogData::clearDataByLogIndex(quint32 index)
{
    bool updated = false;
    typeInfo_.clear();
    LogDataMutableIterator it(data_);
    while (it.hasNext()) {
        it.next();
        if (it.value().logindex == index) {
            it.remove();
            updated = true;
        } else {
            auto &info = typeInfo_[it.value().type];
            ++info.first;
            info.second += getLogDataEntryStringSize(it.value());
        }
    }
    if (updated)
        emit dataUpdated();
}

void LogData::addLines(QStringList lines, LogDataType type, quint32 index,
                       LogRangeCheckType rangeCheck)
{
    if (lines.empty())
        return;

    Q_ASSERT(type < NUM_LOG_TYPES || type == LOG_TYPE_MIXED);
    const int kCurrentYearOffset = QDateTime::currentDateTime().date().year() - 1900;
    QDateTime range[2];
    if (data_.isEmpty())
        rangeCheck = LogRangeCheckType::NONE;
    switch (rangeCheck) {
    case LogRangeCheckType::MIN_TO_MAX:
        range[0] = QDateTime::fromMSecsSinceEpoch(data_.firstKey() >> 20);
        range[1] = QDateTime::fromMSecsSinceEpoch(data_.lastKey() >> 20);
        break;
    case LogRangeCheckType::MIN_TO_NOW:
        range[0] = QDateTime::fromMSecsSinceEpoch(data_.firstKey() >> 20);
        range[1] = QDateTime::currentDateTimeUtc();
        range[1].setTimeSpec(Qt::LocalTime);
        break;
    default:
        break;
    }
    for (auto &line : qAsConst(lines)) {
        processLine(line, type, index, kCurrentYearOffset,
            (rangeCheck != LogRangeCheckType::NONE) ? range : nullptr);
    }
    emit dataUpdated();
}

void LogData::processLine(const QString &line, LogDataType type, quint32 index, int yoffset,
                          QDateTime *range)
{
    if (line.isEmpty())
        return;

    // Verify type.
    QString processingLine;
    if (line.startsWith("G [")) {
        Q_ASSERT(type == LOG_TYPE_MIXED);
        processingLine = line.mid(2);
        type = LOG_TYPE_GUI;
    } else if (line.startsWith("E [")) {
        Q_ASSERT(type == LOG_TYPE_MIXED);
        type = LOG_TYPE_ENGINE;
        processingLine = line.mid(2);
    } else if (line.startsWith("S [")) {
        Q_ASSERT(type == LOG_TYPE_MIXED);
        type = LOG_TYPE_SERVICE;
        processingLine = line.mid(2);
    } else if (line.startsWith("===") || line.startsWith("---")) {
        type = LOG_TYPE_AUX;
        processingLine = line;
    } else {
        Q_ASSERT(type != LOG_TYPE_MIXED);
        processingLine = line;
    }

    int processingLineOffset = 0;
    QDateTime datetime{ lastDateTime_ };
    if (type != LOG_TYPE_AUX) {
        ++processingLineOffset;
        if (isYearInDatePresent(processingLine)) {
            const auto datestr = processingLine.mid(processingLineOffset, 19);
            processingLineOffset += 19;
            datetime = QDateTime::fromString(datestr, "ddMMyy hh:mm:ss:zzz").addYears(100);
        }
        else {
            const auto datestr = processingLine.mid(processingLineOffset, 17);
            processingLineOffset += 17;
            datetime = QDateTime::fromString(datestr, "ddMM hh:mm:ss:zzz").addYears(yoffset);
        }
    }
    if (range && (datetime < range[0] || datetime > range[1]))
        return;
    if (lastDateTime_ != datetime) {
        lastDateTime_ = datetime;
        timestampCounter_ = 0;
    }

    // In QMultiMap, elements with the same key will be placed in a reverse order.
    // https://doc.qt.io/qt-5/qmap-iterator.html#details
    // To deal with the issue, we create a compound key: 44 bits for a timestamp, 2 bits for the
    // source, and 18 additional lower bits for a "timestamp" (line serial number). We assume the
    // maximum number of lines with unique timestamp never exceeds 262143, which is quite reasonable
    // (it looks infeasible to output 250k+ lines to a log file within 1 ms).
    const auto key = (static_cast<quint64>(datetime.toMSecsSinceEpoch()) << 20)
        | (static_cast<quint64>(type & 3) << 18) | qMin(timestampCounter_++, 0x3ffff);

    LogDataEntry entry;
    entry.type = type;
    entry.logindex = index;
    entry.timestamp = datetime.toString("dd.MM.yy hh:mm:ss:zzz");
    if (type != LOG_TYPE_AUX) {
        entry.label = processingLine.mid(processingLineOffset, 11).trimmed();
        processingLineOffset += 12;
    }
    entry.text = processingLine.mid(processingLineOffset).trimmed();

    auto &info = typeInfo_[type];
    ++info.first;
    info.second += getLogDataEntryStringSize(entry);
    data_.insert(key, entry);
}
