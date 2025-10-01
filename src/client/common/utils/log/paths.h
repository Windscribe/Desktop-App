#pragma once
#include <QString>

namespace log_utils
{

// All information on log paths and their file names is located here, so it is not scattered throughout the entire project
namespace paths
{
    // Get log file path with ind.
    // For example, if ind = 0 return "client.log", if ind == 1, return client.1.log and so on
    QString clientLogLocation(bool previous = false);
    QString cliLogLocation(bool previous = false);
    QString serviceLogLocation(bool previous = false);
    QString wireguardServiceLogLocation(bool previous = false);
    QString installerLogLocation();

    QString clientLogFolder();
    QString serviceLogFolder();

    // Delete all unused old logs from previous versions of the program
    void deleteOldUnusedLogs();

}  // namespace paths
}  // namespace log_utils
