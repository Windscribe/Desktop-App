#pragma once

#include <string>

// installer settings
class Settings
{
public:
    static Settings& instance()
    {
        static Settings settings;
        return settings;
    }

    // Returned path will never have a trailing path separator.
    std::wstring getPath() const;
    void setPath(const std::wstring &path);
    void setCreateShortcut(bool create_shortcut);
    bool getCreateShortcut() const;
    void setAutoStart(bool autostart);
    bool getAutoStart() const;
    void setFactoryReset(bool autostart);
    bool getFactoryReset() const;
    void setCredentials(const std::wstring &username, const std::wstring &password);

    void readFromRegistry();
    void writeToRegistry() const;

private:
    std::wstring path_;
    std::wstring username_;
    std::wstring password_;
    bool isCreateShortcut_ = true;
    bool isAutoStart_ = true;
    bool isFactoryReset_ = false;

    explicit Settings();
    ~Settings() = default;
};
