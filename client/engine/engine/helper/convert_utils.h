#pragma once

#include <QString>
#include <QStringList>
#include <algorithm>

namespace convert_utils {

#ifdef Q_OS_WIN
typedef std::wstring universal_string;
#else
typedef std::string universal_string;
#endif

inline universal_string toStdString(const QString &str)
{
#ifdef Q_OS_WIN
    return str.toStdWString();
#else
    return str.toStdString();
#endif
}

inline QString toQString(const universal_string &str)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(str);
#else
    return QString::fromStdString(str);
#endif
}

inline std::vector<universal_string> toStdVector(const QStringList &list)
{
    std::vector<universal_string> result;
    result.reserve(list.size());
    std::transform(list.cbegin(), list.cend(), std::back_inserter(result),
        [](const QString &s) {
            return toStdString(s);
    });
    return result;
}

inline QStringList toStringList(const  std::vector<universal_string> &vector)
{
    QStringList result;
    result.reserve(vector.size());
    std::transform(vector.cbegin(), vector.cend(), std::back_inserter(result),
                   [](const universal_string &s) {
                       return toQString(s);
                   });
    return result;
}



} // namespace convert_utils
