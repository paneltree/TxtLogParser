#ifndef CORE_APPUTILS_H
#define CORE_APPUTILS_H

#include <string>
#include <filesystem>
#include <cstdlib>

namespace Core {

/**
 * @brief Pure C++ utility class for application paths
 * 
 * This class provides cross-platform path handling for macOS, Windows, and Linux.
 */
class AppUtils {
public:
    // Get the logs directory path
    static std::string getLogsDir() {
        std::string path;
        #if defined(_WIN32) || defined(_WIN64)
            path = getAppDataDir() + "\\TxtLogParser\\Logs\\";
        #elif defined(__APPLE__)
            path = getHomeDir() + "/Library/Logs/TxtLogParser/";
        #else // Linux
            path = getHomeDir() + "/.local/share/TxtLogParser/logs/";
        #endif
        ensureDirExists(path);
        return path;
    }
    
    // Get the application support directory path
    static std::string getAppSupportDir() {
        std::string path;
        #if defined(_WIN32) || defined(_WIN64)
            path = getAppDataDir() + "\\TxtLogParser\\";
        #elif defined(__APPLE__)
            path = getHomeDir() + "/Library/Application Support/TxtLogParser/";
        #else // Linux
            path = getHomeDir() + "/.config/TxtLogParser/";
        #endif
        ensureDirExists(path);
        return path;
    }
    
    // Get the path for the application log file
    static std::string getApplicationLogPath() {
        return getLogsDir() + getPathSeparator() + "application.log";
    }
    
    // Get the path for the troubleshooting log file
    static std::string getTroubleshootingLogPath() {
        return getLogsDir() + getPathSeparator() + "troubleshooting.log";
    }
    
    // Get the path for the workspaces file
    static std::string getWorkspacesFilePath() {
        return getAppSupportDir() + getPathSeparator() + "workspaces.json";
    }

private:
    // Get the home directory
    static std::string getHomeDir() {
        #if defined(_WIN32) || defined(_WIN64)
            const char* homeDir = std::getenv("USERPROFILE");
        #else // macOS and Linux
            const char* homeDir = std::getenv("HOME");
        #endif
        return homeDir ? std::string(homeDir) : "";
    }
    
    // Get the Windows AppData directory
    #if defined(_WIN32) || defined(_WIN64)
    static std::string getAppDataDir() {
        const char* appData = std::getenv("APPDATA");
        return appData ? std::string(appData) : getHomeDir() + "\\AppData\\Roaming";
    }
    #endif
    
    // Get platform-specific path separator
    static std::string getPathSeparator() {
        #if defined(_WIN32) || defined(_WIN64)
            return "\\";
        #else
            return "/";
        #endif
    }
    
    // Ensure a directory exists, creating it if necessary
    static void ensureDirExists(const std::string& path) {
        std::filesystem::path dirPath(path);
        if (!std::filesystem::exists(dirPath)) {
            std::filesystem::create_directories(dirPath);
        }
    }
};

} // namespace Core

#endif // CORE_APPUTILS_H 