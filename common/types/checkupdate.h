#ifndef TYPES_CHECKUPDATE_H
#define TYPES_CHECKUPDATE_H

#include <QString>
#include <QJsonObject>
#include <QSharedDataPointer>
#include "utils/protobuf_includes.h"

namespace types {

class CheckUpdateData : public QSharedData
{
public:
    CheckUpdateData() : isInitialized_(false) {}

    CheckUpdateData(const CheckUpdateData &other)
        : QSharedData(other),
          isInitialized_(other.isInitialized_),
          cui_(other.cui_) {}
    ~CheckUpdateData() {}

    bool isInitialized_;
    ProtoTypes::CheckUpdateInfo cui_;  // only this data is saved in settings
};

// implicitly shared class CheckUpdate
class CheckUpdate
{
public:
    explicit CheckUpdate() : d(new CheckUpdateData) {}
    CheckUpdate(const CheckUpdate &other) : d (other.d) {}

    bool initFromJson(QJsonObject &json, QString &outErrorMessage);
    void initFromProtoBuf(const ProtoTypes::CheckUpdateInfo &cui);
    ProtoTypes::CheckUpdateInfo getProtoBuf() const;

    QString getUrl() const;
    QString getSha256() const;
    bool isInitialized() const;

private:
    QSharedDataPointer<CheckUpdateData> d;
};

} // types namespace

#endif // TYPES_CHECKUPDATE_H
