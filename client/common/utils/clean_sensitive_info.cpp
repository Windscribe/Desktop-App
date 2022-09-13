#include "clean_sensitive_info.h"
#include <QStandardPaths>
#include <QString>
#include <string>
#include <vector>

namespace Utils {

namespace {

template<typename T> T QStringCast(QString source) { return source; }
template<> std::string QStringCast(QString source) { return source.toStdString(); }
template<> std::wstring QStringCast(QString source) { return source.toStdWString(); }

#define REGISTER_SENSITIVE_REPLACEMENT(x) \
    replacements.push_back(std::make_pair( \
        QStringCast<T>(QStandardPaths::writableLocation(QStandardPaths::x)), \
        QStringCast<T>(QString("%" #x "%").toUpper())))

template<typename T>
const std::vector<std::pair<T, T>> &GetSensitiveReplacements()
{
    static std::vector<std::pair<T,T>> replacements;
    static bool is_replacements_init = false;
    if (!is_replacements_init) {
        // This will clean most home-related OS paths ("~" and "C:/Users/<USER>").
        REGISTER_SENSITIVE_REPLACEMENT(HomeLocation);
        // This is important for Linux: cleans "/run/user/<USER>". On Mac and Windows, probably is
        // under "~" or "C:/Users/<USER>".
        REGISTER_SENSITIVE_REPLACEMENT(RuntimeLocation);
        // This is important for Android: cleans "<USER>". On desktop OSes, most likely is under "~"
        // or "C:/Users/<USER>".
        REGISTER_SENSITIVE_REPLACEMENT(GenericDataLocation);
        is_replacements_init = true;
    }
    return replacements;
}

#undef REGISTER_SENSITIVE_REPLACEMENT
}  // namespace

template<typename T>
typename std::enable_if<std::is_base_of<QString,T>::value>::type
ReplaceInString(T& str, const std::pair<T, T>& rep)
{
    str.replace(rep.first, rep.second);
}

template<typename T>
typename std::enable_if<
    std::is_base_of<std::basic_string<typename T::traits_type::char_type>, T>::value>::type
ReplaceInString(T& str, const std::pair<T,T>& rep)
{
    const auto start_pos = str.find(rep.first);
    if (start_pos != T::npos)
        str.replace(start_pos, rep.first.length(), rep.second);
}

template <typename T>
T CleanSensitiveInfoHelper<T>::process()
{
    T result{ value_ };
    for (const auto &replacement : GetSensitiveReplacements<T>())
        ReplaceInString(result, replacement);
    return result;
}

// Explicit template instantiations.
template class CleanSensitiveInfoHelper<QString>;
template class CleanSensitiveInfoHelper<std::string>;
template class CleanSensitiveInfoHelper<std::wstring>;

}  // namespace Utils

