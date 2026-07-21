#pragma once

namespace wsl {

enum INSTALLER_STATE {
    STATE_INIT,
    STATE_EXTRACTING,
    STATE_CANCELED,
    STATE_FINISHED,
    STATE_ERROR,
    STATE_LAUNCHED,
    STATE_EXTRACTED
};

enum INSTALLER_ERROR {
    ERROR_OTHER = 1,
    ERROR_PERMISSION,
    ERROR_KILL,
    ERROR_CONNECT_HELPER,
    ERROR_DELETE,
    ERROR_UNINSTALL,
    ERROR_MOVE_CUSTOM_DIR,
    ERROR_CUSTOM_DIR_NOT_EMPTY,
    ERROR_DELETE_CUSTOM_DIR,
    ERROR_BFE_SERVICE_NOT_RUNNING,
    ERROR_BFE_SERVICE_CONNECTION,
    ERROR_HELPER_INSTALL,
    ERROR_HELPER_START,
    // An unhandled exception escaped the install run (see Installer::executionImpl).
    ERROR_INTERNAL,
    // The bundled 7zr.exe extractor is missing or could not be launched
    // (typically antivirus or Smart App Control interference).
    ERROR_EXTRACT_LAUNCH,
    // Extraction of the app payload failed; the 7z exit code and error output
    // are in installer.log.
    ERROR_EXTRACT_FAILED
};

// Stable identifiers shown in error dialogs ("Error code: ...").  Never translated,
// so a screenshot of the dialog is enough for support to identify the failure.
inline const char *errorCodeName(INSTALLER_ERROR error)
{
    switch (error) {
    case ERROR_OTHER:                   return "OTHER";
    case ERROR_PERMISSION:              return "PERMISSION";
    case ERROR_KILL:                    return "KILL";
    case ERROR_CONNECT_HELPER:          return "CONNECT_HELPER";
    case ERROR_DELETE:                  return "DELETE";
    case ERROR_UNINSTALL:               return "UNINSTALL";
    case ERROR_MOVE_CUSTOM_DIR:         return "MOVE_CUSTOM_DIR";
    case ERROR_CUSTOM_DIR_NOT_EMPTY:    return "CUSTOM_DIR_NOT_EMPTY";
    case ERROR_DELETE_CUSTOM_DIR:       return "DELETE_CUSTOM_DIR";
    case ERROR_BFE_SERVICE_NOT_RUNNING: return "BFE_SERVICE_NOT_RUNNING";
    case ERROR_BFE_SERVICE_CONNECTION:  return "BFE_SERVICE_CONNECTION";
    case ERROR_HELPER_INSTALL:          return "HELPER_INSTALL";
    case ERROR_HELPER_START:            return "HELPER_START";
    case ERROR_INTERNAL:                return "INTERNAL";
    case ERROR_EXTRACT_LAUNCH:          return "EXTRACT_LAUNCH";
    case ERROR_EXTRACT_FAILED:          return "EXTRACT_FAILED";
    }
    return "UNKNOWN";
}

}
