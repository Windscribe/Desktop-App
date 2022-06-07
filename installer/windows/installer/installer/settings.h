#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

// installer settings
class Settings
{
public:
	explicit Settings();
	void setPath(const std::wstring &path);
	std::wstring getPath() const;
	void setCreateShortcut(bool create_shortcut);
	bool getCreateShortcut() const;
	void setInstallDrivers(bool install);
	bool getInstallDrivers() const;
	void setAutoStart(bool autostart);
	bool getAutoStart() const;

	bool readFromRegistry();
	void writeToRegistry() const;

private:
	std::wstring path_;
	bool isCreateShortcut_;
	bool isInstallDrivers_;
	bool isAutoStart_;
};


#endif // SETTINGS_H
