#ifndef PROTOENUMTOSTRING_H
#define PROTOENUMTOSTRING_H

#include <QString>
#include <QHash>
#include "utils/protobuf_includes.h"

class ProtoEnumToString
{
public:
    static ProtoEnumToString &instance()
    {
        static ProtoEnumToString s;
        return s;
    }

    QString toString(const QString &protoEnumName);
    QString toString(ProtoTypes::Protocol p);
    QString toString(ProtoTypes::ProxySharingMode p);
    QString toString(ProtoTypes::OrderLocationType p);
    QString toString(ProtoTypes::LatencyDisplayType p);
    QString toString(ProtoTypes::TapAdapterType p);

    QList< QPair<QString, int> > getEnums(const google::protobuf::EnumDescriptor *descriptor);

private:
    ProtoEnumToString();

    QHash<QString, QString> map_;
};

#endif // PROTOENUMTOSTRING_H
