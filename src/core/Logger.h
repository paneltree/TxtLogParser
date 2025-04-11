#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <functional>
#include <sstream>

namespace Core {

/**
 * @brief Pure C++ class for logging
 * 
 * This class replaces the Qt-dependent Logger with a pure C++ implementation.
 */
class Logger {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    static Logger& getInstance();
    
    // Logging methods
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

    //support info( "test" << 1 << ", b=" << 3)
    template<typename T>
    Logger& operator<<(const T& value) {
        m_ostringstream << value;
        return *this;
    }

    void info() {
        info(m_ostringstream.str());
        m_ostringstream.str("");
    }

    void warning() {
        warning(m_ostringstream.str());
        m_ostringstream.str("");
    }
    
    void error() {
        error(m_ostringstream.str());
        m_ostringstream.str("");
    }

    void critical() {
        critical(m_ostringstream.str());
        m_ostringstream.str("");
    }
    // Log to file
    bool setLogFile(const std::string& filePath);
    void closeLogFile();
    
    // Log callback for UI integration
    using LogCallback = std::function<void(LogLevel, const std::string&)>;
    void setLogCallback(LogCallback callback) { logCallback = callback; }
    
private:
    Logger();
    ~Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void log(LogLevel level, const std::string& message);
    std::string getTimestamp();
    std::string getLevelString(LogLevel level);
    
    std::ofstream logFile;
    std::mutex logMutex;
    LogCallback logCallback;
    std::ostringstream m_ostringstream;
};

} // namespace Core

#endif // CORE_LOGGER_H 