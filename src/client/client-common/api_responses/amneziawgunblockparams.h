#pragma once

#include <QString>
#include <QStringList>

namespace api_responses {

struct AmneziawgUnblockParam
{
    QString id;
    QString title;
    QStringList countries;
    int jc = 0;
    int jmin = 0;
    int jmax = 0;
    int s1 = 0;
    int s2 = 0;
    int s3 = 0;
    int s4 = 0;
    QString h1;
    QString h2;
    QString h3;
    QString h4;
    // I1, I2, I3, I4, I5 (in order).  Only I1 is mandatory, the others may not exist.
    QStringList iValues;

    bool isValid() const;
    void setDefault();
};

QDebug operator<<(QDebug debug, const AmneziawgUnblockParam &param);

class AmneziawgUnblockParams
{
public:
    AmneziawgUnblockParams() = default;
    explicit AmneziawgUnblockParams(const std::string &json);

    bool isValid() const { return !params_.isEmpty(); }
    QStringList presets() const;
    QString getTitleById(const QString &id);

    AmneziawgUnblockParam getUnblockParamForPreset(const QString &preset);

private:
    QList<AmneziawgUnblockParam> params_;
};

} //namespace api_responses
