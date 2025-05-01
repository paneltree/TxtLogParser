#ifndef QTBRIDGE_H
#define QTBRIDGE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QList>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QtGui/qcolor.h>
#include <memory>
#include <vector>
#include "FilterAdapter.h"
#include "SearchAdapter.h"
#include "FileAdapter.h"
#include "qoutputline.h"
#include "../core/WorkspaceData.h"

// Core includes
#include "../core/FilterData.h"
#include "../core/SearchData.h"

// Forward declarations for Core classes
namespace Core {
    class WorkspaceManager;
    class WorkspaceData;
    class FileData;
}

/**
 * @brief Bridge class to connect Qt UI with core logic
 * 
 * This class provides a Qt-friendly interface to the pure C++ core logic.
 * It translates between Qt types and standard C++ types.
 */
class QtBridge : public QObject {
    Q_OBJECT
    
public:
    static QtBridge& getInstance();
    
    // Constructor for non-singleton usage
    QtBridge();
    ~QtBridge();
    
    // Workspace management
    int64_t createWorkspace();
    bool removeWorkspace(int64_t id);
    QVector<int64_t> getAllWorkspaceIds() const;

    
    // Active workspace management
    void setActiveWorkspace(int64_t id);
    int64_t getActiveWorkspace() const;
    void beginWorkspaceUpdate();
    void commitWorkspaceUpdate();
    void rollbackWorkspaceUpdate();
    
    // Workspace properties
    QString getWorkspaceName(int64_t id) const;
    bool setWorkspaceName(int64_t id, const QString& name);
    int getWorkspaceSortIndex(int64_t id) const;
    void setWorkspaceSortIndex(int64_t id, int sortIndex);
    
    // File operations for workspaces
    int32_t addFileToWorkspace(int64_t workspaceId, int32_t fileRow, const QString& filePath);
    bool removeFileFromWorkspace(int64_t workspaceId, int32_t fileId);
    bool getFileInfoFromWorkspace(int64_t workspaceId, int32_t fileIndex, FileInfo& info) const;
    void updateFileRowInWorkspace(int64_t workspaceId, int32_t fileId, int32_t fileRow);
    void updateFileSelectionInWorkspace(int64_t workspaceId, int32_t fileId, bool selected);
    void getFileListFromWorkspace(int64_t workspaceId, const std::function<void(const QList<FileInfo>&)>& callback);
    void beginFileUpdate(int64_t workspaceId);
    void commitFileUpdate(int64_t workspaceId);
    void rollbackFileUpdate(int64_t workspaceId);
    void reloadFilesInWorkspace(int64_t workspaceId);

    // Filter operations for workspaces
    int32_t addFilterToWorkspace(int64_t workspaceId, const FilterConfig& filter);
    bool removeFilterFromWorkspace(int64_t workspaceId, int32_t filterId);
    void getFilterListFrmWorkspace(int64_t workspaceId, const std::function<void(const QList<FilterConfig>&)>& callback);
    QMap<int, int> getFilterMatchCounts(int64_t workspaceId) const;
    void updateFilterRowInWorkspace(int64_t workspaceId, int32_t filterId, int32_t filterRow);
    void updateFilterInWorkspace(int64_t workspaceId, const FilterConfig& filter);
    void beginFilterUpdate(int64_t workspaceId);
    void commitFilterUpdate(int64_t workspaceId);
    void rollbackFilterUpdate(int64_t workspaceId);
    QColor getNextFilterColor(int64_t workspaceId);
    // Search operations for workspaces
    int32_t addSearchToWorkspace(int64_t workspaceId, const SearchConfig& search);
    bool removeSearchFromWorkspace(int64_t workspaceId, int32_t searchId);
    void getSearchListFrmWorkspace(int64_t workspaceId, const std::function<void(const QList<SearchConfig>&)>& callback);
    void updateSearchRowInWorkspace(int64_t workspaceId, int32_t searchId, int32_t searchRow);
    void updateSearchInWorkspace(int64_t workspaceId, const SearchConfig& search);
    void beginSearchUpdate(int64_t workspaceId);
    void commitSearchUpdate(int64_t workspaceId);
    void rollbackSearchUpdate(int64_t workspaceId);
    QColor getNextSearchColor(int64_t workspaceId);
    
    


    // Output operations for workspaces
    QList<QOutputLine> getOutputStringList(int64_t workspaceId) const;
    bool getNextMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getNextMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    QMap<int, int> getSearchMatchCounts(int64_t workspaceId) const;
    
    
    // Workspace data
    bool loadWorkspaces();
    bool saveWorkspaces();
    
    // Logging
    void logDebug(const QString& message);
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);
    void logCritical(const QString& message);
    void logMessage(const QString& message) { logInfo(message); }
    
    // Troubleshooting logging
    void troubleshootingLog(const QString& category, const QString& operation, const QString& message);
    void troubleshootingLogMessage(const QString& message);
    void troubleshootingLogFilterOperation(const QString& operation, const QString& filterString, const QString& message);
    
signals:    
    // Log signals
    void logMessage(const QString& message, int level);
    void troubleshootingLogSignal(const QString& category, const QString& operation, const QString& message);
    
private:
    // Prevent copying
    QtBridge(const QtBridge&) = delete;
    QtBridge& operator=(const QtBridge&) = delete;
    
    // Core components
    std::unique_ptr<Core::WorkspaceManager> workspaceManager;
    
    // Storage for filters and searches
    std::vector<Core::FilterDataPtr> filters;
    std::vector<Core::SearchDataPtr> searches;
    
    // Workspace storage
    std::vector<std::shared_ptr<Core::WorkspaceData>> workspaces;
    int activeWorkspaceId;
    
    // Helper methods
    void setupLogCallbacks();
    
    // String conversion helpers
    QString toQString(const std::string& str) const;
    std::string fromQString(const QString& str) const;
    
    // Color conversion helpers
    QColor colorFromCode(int code);
    int codeFromColor(const QColor& color);
};

#endif // QTBRIDGE_H
