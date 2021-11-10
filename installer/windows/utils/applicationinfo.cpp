#include "applicationinfo.h"
#include "../../../common/version/windscribe_version.h"

ApplicationInfo::ApplicationInfo()
{

}

std::wstring ApplicationInfo::getName() const
{
	return name;
}

std::wstring ApplicationInfo::getVersion() const
{
	return WINDSCRIBE_VERSION_STR_UNICODE;
}

std::wstring ApplicationInfo::getUninstall() const
{
	return uninstall;
}

std::wstring  ApplicationInfo::getPublisher() const
{
	return publisher;
}

std::wstring ApplicationInfo::getId() const
{
	return id;
}

std::wstring ApplicationInfo::getPublisherUrl() const
{
	return publisherUrl;
}

std::wstring ApplicationInfo::getSupportUrl() const
{
	return supportURL;
}

std::wstring ApplicationInfo::getUpdateUrl() const
{
	return updateURL;
}

bool ApplicationInfo::appIsRunning()
{
 HWND hwnd;
 bool result;
 result = true;
 hwnd = FindWindow(L"Qt5QWindowIcon", L"Windscribe");
  if (hwnd != nullptr)
  {
    while(1)
    {
      int msgboxID =  MessageBox(
                  nullptr,
                  static_cast<LPCWSTR>(L"Close Windscribe to continue. Please note, your connection will not be protected while the application is off.'"),
                static_cast<LPCWSTR>(L"Windscribe"),
                MB_ICONINFORMATION | MB_RETRYCANCEL);

      if (msgboxID == IDCANCEL)
      {
        // user clicked Cancel
        result = true;
        break;
      }
      else
      {
        hwnd = FindWindow(L"Qt5QWindowIcon", L"Windscribe");
        if (hwnd==nullptr)
        {
          result = false;
          break;
        }
      }
    }
  }
  else
  {
   result = false;
  }
  return result;
}
