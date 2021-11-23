#ifndef EXECUTABLE_SIGNATURE_DEFS_H
#define EXECUTABLE_SIGNATURE_DEFS_H

#define WINDOWS_CERT_SUBJECT_NAME L"Windscribe Limited"
#define MACOS_CERT_DEVELOPER_ID "Developer ID Application: Windscribe Limited (GYZJYS7XUG)"

// NOTES:
// - What does the [certificateName] section in common\utils\hardcodedsettings.ini do?
//   - It doesn't look like it's used anywhere but in the hardcodedsettings.cpp file.
// - Do we need to instruct them to remove/edit BUILD_DEVELOPER_MAC in build_all.py?

#endif // EXECUTABLE_SIGNATURE_DEFS_H
