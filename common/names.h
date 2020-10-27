#ifndef FILENAMES_H
#define FILENAMES_H

#include <string>

const std::string INSTALLER_FILENAME_MAC = "WindscribeInstaller";
const std::string INSTALLER_FILENAME_MAC_APP = INSTALLER_FILENAME_MAC + ".app";
const std::string INSTALLER_FILENAME_MAC_DMG = INSTALLER_FILENAME_MAC + ".dmg";

const std::string INSTALLER_INNER_BINARY_MAC = "Contents/MacOS/installer";

const std::string LAUNCHER_BUNDLE_ID = "com.windscribe.launcher.macos";
const std::string GUI_BUNDLE_ID = "com.windscribe.gui.macos";
const std::string HELPER_BUNDLE_ID = "com.windscribe.helper.macos";

const std::string HELPER_BUNDLE_ID_PATH_FROM_ENGINE = "Contents/Library/LaunchServices/" + HELPER_BUNDLE_ID;


#endif // FILENAMES_H
