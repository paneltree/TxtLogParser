#include "QtBridge.h"
#include "../core/LoggerBridge.h"
#include "../core/TimeUtils.h"
#include "../core/JsonUtils.h"
#include "../core/FileSystem.h"
#include "../core/AppUtils.h"
#include "../core/Logger.h"
#include "../core/StringConverter.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "../core/WorkspaceManager.h"
#include "../core/WorkspaceData.h"
#include "../core/FileData.h"
#include "../core/FilterData.h"
#include "../core/SearchData.h"
#include "bridge/FileAdapter.h"
#include "bridge/FilterAdapter.h"
#include "bridge/SearchAdapter.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

QtBridge& QtBridge::getInstance() {
    static QtBridge instance;
    return instance;
}

QtBridge::QtBridge() : activeWorkspaceId(-1) {
    // Initialize core components
    workspaceManager = std::make_unique<Core::WorkspaceManager>();
    
    // Initialize log files
    Core::Logger::getInstance().setLogFile(Core::AppUtils::getApplicationLogPath());
    
    // Initialize LoggerBridge
    Core::LoggerBridge::getInstance().initialize(
        Core::AppUtils::getApplicationLogPath(),
        Core::AppUtils::getTroubleshootingLogPath(),
        true,
        Core::LogLevel::INFO
    );
    
    // Setup log callbacks
    setupLogCallbacks();
    
    // Setup a log handler to write logs to file
    Core::Logger::getInstance().setLogCallback([](Core::Logger::LogLevel level, const std::string& message) {
        // Additional file logging is already handled by the Logger class
    });
    
    // Setup LoggerBridge callback for UI updates
    Core::LoggerBridge::getInstance().setLogCallback([this](Core::LogLevel level, const std::string& message) {
        emit logMessage(Core::StringConverter::toQString(message), static_cast<int>(level));
    });        
}

QtBridge::~QtBridge() {
    // Shutdown LoggerBridge
    Core::LoggerBridge::getInstance().shutdown();
}

void QtBridge::setupLogCallbacks() {
}

int64_t QtBridge::createWorkspace() {
    return workspaceManager->createWorkspace();
}

bool QtBridge::removeWorkspace(int64_t id) {
    return workspaceManager->removeWorkspace(static_cast<size_t>(id));
}

QVector<int64_t> QtBridge::getAllWorkspaceIds() const {
    QVector<int64_t> ids;
    for (auto id : workspaceManager->getAllWorkspaceIds()) {
        ids.append(id);
    }
    return ids;
}

void QtBridge::setActiveWorkspace(int64_t id) {
    logInfo("QtBridge::setActiveWorkspace Setting active workspace to id: " + QString::number(id));
    workspaceManager->setActiveWorkspace(id);
}

int64_t QtBridge::getActiveWorkspace() const {
    return workspaceManager->getActiveWorkspace();
}

void QtBridge::beginWorkspaceUpdate() {
    workspaceManager->beginWorkspaceUpdate();
}

void QtBridge::commitWorkspaceUpdate() {
    workspaceManager->commitWorkspaceUpdate();
}

void QtBridge::rollbackWorkspaceUpdate() {
    workspaceManager->rollbackWorkspaceUpdate();
}

QString QtBridge::getWorkspaceName(int64_t id) const {    
    auto workspace = workspaceManager->getWorkspace(id);
    if (workspace) {
        return toQString(workspace->getName());
    }
    return QString();
}

bool QtBridge::setWorkspaceName(int64_t id, const QString& name) {    
    std::string stdName = fromQString(name);
    bool success = workspaceManager->setWorkspaceName(id, stdName);
    
    if (success) {
        // Get the actual name that was set (might be different if a unique name was generated)
        auto workspace = workspaceManager->getWorkspace(id);
        if (workspace) {
            QString actualName = toQString(workspace->getName());
            logInfo("Set workspace " + QString::number(id) + " name to: " + actualName);
            return true;
        }
    }
    
    logError("Failed to set workspace name for id " + QString::number(id));
    return false;
}

int QtBridge::getWorkspaceSortIndex(int64_t id) const {
    return workspaceManager->getWorkspaceSortIndex(id);
}

void QtBridge::setWorkspaceSortIndex(int64_t id, int sortIndex) {
    workspaceManager->setWorkspaceSortIndex(id, sortIndex);
}

////////////////////////////////////////////////////////////
// File management
////////////////////////////////////////////////////////////

int32_t QtBridge::addFileToWorkspace(int64_t workspaceId, int32_t fileRow, const QString& filePath) {
    return workspaceManager->addFileToWorkspace(workspaceId, fileRow, filePath.toStdString());
}

bool QtBridge::removeFileFromWorkspace(int64_t workspaceId, int32_t fileId) {   
    return workspaceManager->removeFileFromWorkspace(workspaceId, fileId);
}

bool QtBridge::getFileInfoFromWorkspace(int64_t workspaceId, int32_t fileIndex, FileInfo& info) const
{
    auto file = workspaceManager->getFileData(workspaceId, fileIndex);
    if (file) {
        return FileAdapter::getInstance().toFileInfo(file, info);
    }
    return false;
}

void QtBridge::updateFileRowInWorkspace(int64_t workspaceId, int32_t fileId, int32_t fileRow) {
    workspaceManager->updateFileRow(workspaceId, fileId, fileRow);
}

void QtBridge::updateFileSelectionInWorkspace(int64_t workspaceId, int32_t fileId, bool selected){
    workspaceManager->updateFileSelection(workspaceId, fileId, selected);
}

void QtBridge::getFileListFromWorkspace(int64_t workspaceId, const std::function<void(const QList<FileInfo>&)>& callback) {
    QList<FileInfo> fileList;
    for(const auto& fileData : workspaceManager->getFileDataList(workspaceId)){
        FileInfo fileInfo(QString::fromStdString(fileData->getFilePath()),
                             QString::fromStdString(fileData->getFileName()),
                             std::chrono::system_clock::from_time_t(fileData->getModifiedTime()),
                             fileData->getFileSize(),
                             fileData->isSelected(),
                             fileData->getFileId(),
                             fileData->getFileRow(),
                             fileData->isExists());
        fileList.append(fileInfo);
    }
    callback(fileList);
}

void QtBridge::beginFileUpdate(int64_t workspaceId) {
    workspaceManager->beginFileUpdate(workspaceId);
}

void QtBridge::commitFileUpdate(int64_t workspaceId) {
    workspaceManager->commitFileUpdate(workspaceId);
}

void QtBridge::rollbackFileUpdate(int64_t workspaceId) {
    workspaceManager->rollbackFileUpdate(workspaceId);
}

void QtBridge::reloadFilesInWorkspace(int64_t workspaceId) {
    workspaceManager->reloadFilesInWorkspace(workspaceId);
}

////////////////////////////////////////////////////////////
// Filter management
////////////////////////////////////////////////////////////

int32_t QtBridge::addFilterToWorkspace(int64_t workspaceId, const FilterConfig& filter) {
    Core::FilterData filterData(
        filter.filterId,
        filter.filterRow,
        filter.filterPattern.toStdString(),
        filter.caseSensitive,
        filter.wholeWord,
        filter.isRegex,
        filter.enabled,
        filter.color.name().toStdString()
    );
    return workspaceManager->addFilterToWorkspace(workspaceId, filterData);
}

bool QtBridge::removeFilterFromWorkspace(int64_t workspaceId, int32_t filterId) {
    return workspaceManager->removeFilterFromWorkspace(workspaceId, filterId);
}

void QtBridge::getFilterListFrmWorkspace(int64_t workspaceId, const std::function<void(const QList<FilterConfig>&)>& callback) {
    QList<FilterConfig> filterList;
    for(auto filter : workspaceManager->getFilterDataList(workspaceId)){
        filterList.append(FilterAdapter::getInstance().toFilterConfig(*filter));
    }
    callback(filterList);
}

QMap<int, int> QtBridge::getFilterMatchCounts(int64_t workspaceId) const {
    std::map<int32_t, int32_t> matchCounts = workspaceManager->getFilterMatchCounts(workspaceId);
    QMap<int, int> result;
    for (const auto& [filterId, matchCount] : matchCounts) {
        result[filterId] = matchCount;
    }
    return result;
}

void QtBridge::updateFilterRowsInWorkspace(int64_t workspaceId, QList<qint32> filterIds)
{
    std::list<int32_t> filterIds2;
    for (auto filterId : filterIds) {
        filterIds2.push_back(filterId);
    }
    workspaceManager->updateFilterRows(workspaceId, filterIds2);
}

void QtBridge::updateFilterInWorkspace(int64_t workspaceId, const FilterConfig& filter) {
    Core::FilterData filterData(
        filter.filterId,
        filter.filterRow,
        filter.filterPattern.toStdString(),
        filter.caseSensitive,
        filter.wholeWord,
        filter.isRegex,
        filter.enabled,
        filter.color.name().toStdString()
    );
    workspaceManager->updateFilter(workspaceId, filterData);
}

void QtBridge::beginFilterUpdate(int64_t workspaceId) {
    workspaceManager->beginFilterUpdate(workspaceId);
}

void QtBridge::commitFilterUpdate(int64_t workspaceId) {
    workspaceManager->commitFilterUpdate(workspaceId);
}

void QtBridge::rollbackFilterUpdate(int64_t workspaceId) {
    workspaceManager->rollbackFilterUpdate(workspaceId);
}

QColor QtBridge::getNextFilterColor(int64_t workspaceId) {
    std::string color = workspaceManager->getNextFilterColor(workspaceId);
    //create QColor from string
    if (color.empty()) {
        return QColor();
    }
    // Check if the color string is in hex format
    if (color[0] == '#') {
        return QColor(color.c_str());
    }
    // If not, assume it's in RGB format
    // Split the string by commas
    std::string rStr = color.substr(0, color.find(','));
    std::string gStr = color.substr(color.find(',') + 1, color.find_last_of(',') - color.find(',') - 1);
    std::string bStr = color.substr(color.find_last_of(',') + 1);
    // Convert to integers
    int r = std::stoi(rStr);
    return QColor(color.c_str());
}

////////////////////////////////////////////////////////////
// Search management
////////////////////////////////////////////////////////////

int32_t QtBridge::addSearchToWorkspace(int64_t workspaceId, const SearchConfig& search) {
    Core::SearchData searchData(
        search.searchId,
        search.searchRow,
        search.searchPattern.toStdString(),
        search.caseSensitive,
        search.wholeWord,
        search.isRegex,
        search.enabled,
        search.color.name().toStdString()
    );
    return workspaceManager->addSearchToWorkspace(workspaceId, searchData);
}

bool QtBridge::removeSearchFromWorkspace(int64_t workspaceId, int32_t searchId) {
    return workspaceManager->removeSearchFromWorkspace(workspaceId, searchId);
}

void QtBridge::getSearchListFrmWorkspace(int64_t workspaceId, const std::function<void(const QList<SearchConfig>&)>& callback) {
    QList<SearchConfig> searchList;
    for(auto search : workspaceManager->getSearchDataList(workspaceId)){
        searchList.append(SearchAdapter::getInstance().toSearchConfig(*search));
    }
    callback(searchList);
}

void QtBridge::updateSearchRowsInWorkspace(int64_t workspaceId, QList<qint32> searchIds) {
    std::list<int32_t> stdSearchIds;
    for (const auto& id : searchIds) {
        stdSearchIds.push_back(id);
    }
    workspaceManager->updateSearchRows(workspaceId, stdSearchIds);
}

void QtBridge::updateSearchInWorkspace(int64_t workspaceId, const SearchConfig& search) {
    Core::SearchData searchData(
        search.searchId,
        search.searchRow,
        search.searchPattern.toStdString(),
        search.caseSensitive,
        search.wholeWord,
        search.isRegex,
        search.enabled,
        search.color.name().toStdString()
    );
    workspaceManager->updateSearch(workspaceId, searchData);
}

void QtBridge::beginSearchUpdate(int64_t workspaceId) {
    workspaceManager->beginSearchUpdate(workspaceId);
}

void QtBridge::commitSearchUpdate(int64_t workspaceId) {
    workspaceManager->commitSearchUpdate(workspaceId);
}

void QtBridge::rollbackSearchUpdate(int64_t workspaceId) {
    workspaceManager->rollbackSearchUpdate(workspaceId);
}

QColor QtBridge::getNextSearchColor(int64_t workspaceId) {
    std::string color = workspaceManager->getNextSearchColor(workspaceId);
    //create QColor from string
    if (color.empty()) {
        return QColor();
    }
    // Check if the color string is in hex format
    if (color[0] == '#') {
        return QColor(color.c_str());
    }
    // If not, assume it's in RGB format
    // Split the string by commas
    std::string rStr = color.substr(0, color.find(','));
    std::string gStr = color.substr(color.find(',') + 1, color.find_last_of(',') - color.find(',') - 1);
    std::string bStr = color.substr(color.find_last_of(',') + 1);
    // Convert to integers
    int r = std::stoi(rStr);
    return QColor(color.c_str());
}

////////////////////////////////////////////////////////////
// Workspace management
////////////////////////////////////////////////////////////

bool QtBridge::loadWorkspaces() {
    bool success = workspaceManager->loadWorkspaces();
    if (success) {
        logInfo(QString("Successfully loaded workspaces"));
    } else {
        logError("Failed to load workspaces");
    }
    return success;
}

bool QtBridge::saveWorkspaces() {
    bool success = workspaceManager->saveWorkspaces();
    if (success) {
        logInfo(QString("Successfully saved workspaces"));
    } else {
        logError("Failed to save workspaces");
    }
    return success;
}

void QtBridge::logDebug(const QString& message) {
    emit logMessage(message, 0); // Debug level
    Core::LoggerBridge::getInstance().debug(Core::StringConverter::fromQString(message));
}

void QtBridge::logInfo(const QString& message) {
    emit logMessage(message, 1); // Info level
    Core::LoggerBridge::getInstance().info(Core::StringConverter::fromQString(message));
}

void QtBridge::logWarning(const QString& message) {
    emit logMessage(message, 2); // Warning level
    Core::LoggerBridge::getInstance().warning(Core::StringConverter::fromQString(message));
}

void QtBridge::logError(const QString& message) {
    emit logMessage(message, 3); // Error level
    Core::LoggerBridge::getInstance().error(Core::StringConverter::fromQString(message));
}

void QtBridge::logCritical(const QString& message) {
    emit logMessage(message, 4); // Critical level
    Core::LoggerBridge::getInstance().critical(Core::StringConverter::fromQString(message));
}

void QtBridge::troubleshootingLog(const QString& category, const QString& operation, const QString& message) {
    Core::LoggerBridge::getInstance().troubleshootingLog(
        Core::StringConverter::fromQString(category),
        Core::StringConverter::fromQString(operation),
        Core::StringConverter::fromQString(message)
    );
    
    // 发送信号通知UI
    emit troubleshootingLogSignal(category, operation, message);
}

void QtBridge::troubleshootingLogMessage(const QString& message) {
    Core::LoggerBridge::getInstance().troubleshootingLogMessage(
        Core::StringConverter::fromQString(message)
    );
    
    // 发送信号通知UI
    emit troubleshootingLogSignal("INFO", "MESSAGE", message);
}

void QtBridge::troubleshootingLogFilterOperation(const QString& operation, const QString& filterString, const QString& message) {
    Core::LoggerBridge::getInstance().troubleshootingLogFilterOperation(
        Core::StringConverter::fromQString(operation),
        Core::StringConverter::fromQString(message)
    );
    
    // 发送信号通知UI
    emit troubleshootingLogSignal("FILTER", operation, message);
}

QString QtBridge::toQString(const std::string& str) const {
    return QString::fromStdString(str);
}

std::string QtBridge::fromQString(const QString& str) const {
    return str.toStdString();
}

QColor QtBridge::colorFromCode(int code) {
    return QColor::fromRgb(code);
}

int QtBridge::codeFromColor(const QColor& color) {
    return color.rgb();
}



QList<QOutputLine> QtBridge::getOutputStringList(int64_t workspaceId) const {
    std::vector<std::shared_ptr<Core::OutputLine>> coreOutputLines  = workspaceManager->getOutputStringList(workspaceId);
    QList<QOutputLine> result;
    for (const auto& coreOutputLine : coreOutputLines) {
        QOutputLine qOutputLine;    
        qOutputLine.m_fileId = coreOutputLine->getFileId();
        qOutputLine.m_fileRow = coreOutputLine->getFileRow();
        qOutputLine.m_lineIndex = coreOutputLine->getLineIndex();
        for (const auto& coreOutputSubLine : coreOutputLine->getSubLines()) {
            QOutputSubLine qOutputSubLine;
            qOutputSubLine.m_fileId = coreOutputSubLine.getFileId();
            qOutputSubLine.m_content = QString::fromStdString(std::string(coreOutputSubLine.getContent()));
            qOutputSubLine.m_color = QString::fromStdString(coreOutputSubLine.getColor());
            qOutputLine.m_subLines.append(qOutputSubLine);
        }
        result.append(qOutputLine);
    }

    return result;  
}

bool QtBridge::getNextMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return workspaceManager->getNextMatchByFilter(workspaceId, filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}


bool QtBridge::getPreviousMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return workspaceManager->getPreviousMatchByFilter(workspaceId, filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool QtBridge::getNextMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return workspaceManager->getNextMatchBySearch(workspaceId, searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool QtBridge::getPreviousMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                      int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return workspaceManager->getPreviousMatchBySearch(workspaceId, searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

QMap<int, int> QtBridge::getSearchMatchCounts(int64_t workspaceId) const {
    std::map<int32_t, int32_t> matchCounts = workspaceManager->getSearchMatchCounts(workspaceId);
    QMap<int, int> result;
    for (const auto& [searchId, matchCount] : matchCounts) {
        result[searchId] = matchCount;
    }
    return result;
}
