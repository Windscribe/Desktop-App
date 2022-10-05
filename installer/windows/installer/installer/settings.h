#ifndef SETTINGS_H
#define SETTINGS_H

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

    void setPath(const std::wstring &path);
	std::wstring getPath() const;
	void setCreateShortcut(bool create_shortcut);
	bool getCreateShortcut() const;
	void setInstallDrivers(bool install);
	bool getInstallDrivers() const;
	void setAutoStart(bool autostart);
	bool getAutoStart() const;
	void setFactoryReset(bool autostart);
	bool getFactoryReset() const;

	bool readFromRegistry();
	void writeToRegistry() const;

private:
	std::wstring path_;
	bool isCreateShortcut_;
	bool isInstallDrivers_;
	bool isAutoStart_;
	bool isFactoryReset_;

    explicit Settings();
    ~Settings() = default;
};


#endif // SETTINGS_H
