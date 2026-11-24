#pragma once

#include <string>

namespace DeleteUserData
{

// Deletes the settings and data the app and installer leave on the computer, for a user who will
// not be reinstalling (the uninstall confirmation's checkbox, or /DELETEALLDATA):
// - for the interactive user, while impersonating them (see wsl::InteractiveUser): the app's
//   settings and installer keys, the multiple-account detection record, the launch-on-startup
//   entries, and the app's %LOCALAPPDATA% data;
// - for the account the uninstaller runs as, when that is a different account: the same registry
//   entries, which the elevated installer (or an app run as that account) wrote there;
// - machine-wide: the helper's settings key in HKLM, and the Windows Firewall rules Windows created
//   for the app's executable in installationPath.
// Call only after the app has been uninstalled.  Returns true if everything that existed was
// removed.
bool run(const std::wstring &installationPath);

}
