#ifndef CORE_TROUBLESHOOTINGLOGGER_H
#define CORE_TROUBLESHOOTINGLOGGER_H

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Core {

/**
 * @brief Pure C++ class for troubleshooting logging
 * 
 * This class replaces the Qt-dependent TroubleshootingLogger with a pure C++ implementation.
 */
class TroubleshootingLogger {
public:
    // Log categories
    enum class Category {
        FILTER,
        SEARCH,
        FILE,
        WORKSPACE,
        NAVIGATION,
        UI,
        GENERAL
    };
    
    // Log operations
    enum class Operation {
        CREATE,
        UPDATE,
        DELETE,
        LOAD,
        SAVE,
        NAVIGATE,
        PROCESS,
        ERROR
    };
    
    static TroubleshootingLogger& getInstance();
    
    // Logging methods
    void log(Category category, Operation operation, const std::string& message);
    
    // Enable/disable logging
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }
    
    // Log callback for UI integration
    using LogCallback = std::function<void(Category, Operation, const std::string&)>;
    void setLogCallback(LogCallback callback) { logCallback = callback; }
    
    // Category and operation string conversion
    static std::string categoryToString(Category category);
    static std::string operationToString(Operation operation);
    
private:
    TroubleshootingLogger();
    ~TroubleshootingLogger();
    
    // Prevent copying
    TroubleshootingLogger(const TroubleshootingLogger&) = delete;
    TroubleshootingLogger& operator=(const TroubleshootingLogger&) = delete;
    
    bool isEnabled;
    LogCallback logCallback;
};

} // namespace Core

#endif // CORE_TROUBLESHOOTINGLOGGER_H 