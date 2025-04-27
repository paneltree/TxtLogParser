#include <QApplication>
#include <QGuiApplication>
#include <iostream>
#include <QStandardPaths>
#include <filesystem>
#include "mainwindow.h"
#include "../bridge/QtBridge.h"
#include "../core/AppUtils.h"
#include "../core/Logger.h"

void printPaths() {
    auto& logger = Core::Logger::getInstance();
    
    // Print application paths
    logger << "=== Application Paths ===";
    logger.info();
    logger << "Application log path: " << Core::AppUtils::getApplicationLogPath();
    logger.info();
    logger << "Troubleshooting log path: " << Core::AppUtils::getTroubleshootingLogPath();
    logger.info();
    
    // Print Qt settings path
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    logger << "Settings path: " << settingsPath.toStdString();
    logger.info();
    
    // Print workspace configuration path
    std::string workspacePath = Core::AppUtils::getWorkspacesFilePath();
    logger << "Workspace config path: " << workspacePath;
    logger.info();
    
    // Check if workspace file exists
    if (std::filesystem::exists(workspacePath)) {
        logger << "Workspace file exists with size: " << std::filesystem::file_size(workspacePath) << " bytes";
        logger.info();
    } else {
        logger << "Workspace file does not exist";
        logger.info();
    }
    
    // Print application support directory contents
    std::string appSupportDir = Core::AppUtils::getAppSupportDir();
    logger << "\nApplication Support Directory Contents:";
    logger.info();
    for (const auto& entry : std::filesystem::directory_iterator(appSupportDir)) {
        std::ostringstream entryInfo;
        entryInfo << "  " << entry.path().filename().string();
        if (entry.is_regular_file()) {
            entryInfo << " (" << entry.file_size() << " bytes)";
        }
        logger << entryInfo.str();
        logger.info();
    }
    
    logger << "======================";
    logger.info();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序图标
    app.setWindowIcon(QIcon(":/icons/icons/app_icon.png"));
    
    // 设置应用元数据
    QApplication::setApplicationName("TxtLogParser");
    QApplication::setOrganizationName("paneltree");
    QApplication::setOrganizationDomain("github.com/paneltree");
    
    // Initialize the bridge
    QtBridge::getInstance();
    
    // Print all paths
    printPaths();
    
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}