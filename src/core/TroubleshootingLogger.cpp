#include "TroubleshootingLogger.h"
#include "Logger.h"
#include <iostream>

namespace Core {

TroubleshootingLogger& TroubleshootingLogger::getInstance() {
    static TroubleshootingLogger instance;
    return instance;
}

TroubleshootingLogger::TroubleshootingLogger() : isEnabled(true) {
    // Constructor
}

TroubleshootingLogger::~TroubleshootingLogger() {
    // Destructor
}

void TroubleshootingLogger::log(Category category, Operation operation, const std::string& message) {
    if (!isEnabled) {
        return;
    }
    
    std::string categoryStr = categoryToString(category);
    std::string operationStr = operationToString(operation);
    std::string formattedMessage = "[" + categoryStr + "][" + operationStr + "] " + message;
    
    // Log to the main logger
    Logger::getInstance().debug(formattedMessage);
    
    // Call callback if set
    if (logCallback) {
        logCallback(category, operation, message);
    }
}

std::string TroubleshootingLogger::categoryToString(Category category) {
    switch (category) {
        case Category::FILTER:
            return "FILTER";
        case Category::SEARCH:
            return "SEARCH";
        case Category::FILE:
            return "FILE";
        case Category::WORKSPACE:
            return "WORKSPACE";
        case Category::NAVIGATION:
            return "NAVIGATION";
        case Category::UI:
            return "UI";
        case Category::GENERAL:
            return "GENERAL";
        default:
            return "UNKNOWN";
    }
}

std::string TroubleshootingLogger::operationToString(Operation operation) {
    switch (operation) {
        case Operation::CREATE:
            return "CREATE";
        case Operation::UPDATE:
            return "UPDATE";
        case Operation::DELETE:
            return "DELETE";
        case Operation::LOAD:
            return "LOAD";
        case Operation::SAVE:
            return "SAVE";
        case Operation::NAVIGATE:
            return "NAVIGATE";
        case Operation::PROCESS:
            return "PROCESS";
        case Operation::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

} // namespace Core 