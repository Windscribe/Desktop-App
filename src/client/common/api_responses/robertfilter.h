#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>

namespace api_responses {

struct RobertFilter
{
    QString id;
    QString title;
    QString description;
    int status = 0;

    bool initFromJson(const QJsonObject &json);
    bool operator==(const RobertFilter &other) const;
    bool operator!=(const RobertFilter &other) const;

    friend QDataStream& operator <<(QDataStream &stream, const RobertFilter &o);
    friend QDataStream& operator >>(QDataStream &stream, RobertFilter &o);

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

class RobertFilters
{
public:
    RobertFilters(const std::string &json);
    const QVector<RobertFilter> &filters() const { return filters_; }

private:
    QVector<RobertFilter> filters_;

};

} //namespace api_responses
