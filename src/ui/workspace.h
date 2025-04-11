#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QWidget>
#include <QLabel>
#include <QShowEvent>
#include "../bridge/QtBridge.h"
#include "models/filterconfig.h"

class FileListWidget;
class FilterListWidget;
class SearchListWidget;
class OutputDisplayWidget;

class Workspace : public QWidget
{
    Q_OBJECT

public:
    explicit Workspace(int64_t id, QWidget *parent = nullptr);
    ~Workspace();


    // Initialize workspace with data
    void initializeWithData();

    void updateTranslation();
    QString getDisplayName() const;
    int64_t getWorkspaceId() const { return workspaceId; }
    bool setWorkspaceName(const QString &name);
    int getSortIndex() const;
    void setSortIndex(int index);
    void stopLoading();

    QStringList getSelectedFiles() const;

    // Search management
    void addSearch(const QString &query, bool caseSensitive, bool wholeWord, bool regex);

    // Save workspace data
    void saveWorkspaceData();

    void setActive(bool active);

signals:
    void filterAdded(const FilterConfig& filter);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onFilesChanged();
    void onRequestFileIndex(const QString &filePath, int &fileIndex);
    void onFiltersChanged();
    void onSearchsChanged();
    void onFilterMatchCountsUpdated(const QMap<int, int> &matchCounts);

private:
    const int64_t workspaceId;
    QString workspaceName;
    //QLabel *nameLabel;
    
    FileListWidget *fileListWidget;
    FilterListWidget *filterListWidget;
    SearchListWidget *searchListWidget;
    OutputDisplayWidget *outputDisplay;
    
    // Bridge to core functionality
    QtBridge& bridge;
    
    // Workspace index in the bridge
};

#endif // WORKSPACE_H
