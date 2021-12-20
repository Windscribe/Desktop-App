#ifndef CLEAN_SENSITIVE_INFO_H
#define CLEAN_SENSITIVE_INFO_H

namespace Utils {

template<typename T>
class CleanSensitiveInfoHelper
{
public:
    explicit CleanSensitiveInfoHelper(const T& value) : value_(value) {}
    CleanSensitiveInfoHelper(const CleanSensitiveInfoHelper&) = delete;
    CleanSensitiveInfoHelper& operator=(const CleanSensitiveInfoHelper&) = delete;
    T process();

private:
    T value_;
};

template<typename T>
T cleanSensitiveInfo(const T &value)
{
    return CleanSensitiveInfoHelper<T>(value).process();
}

}  // namespace Utils

#endif  // CLEAN_SENSITIVE_INFO_H
