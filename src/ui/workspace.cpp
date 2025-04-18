#include "workspace.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include "widgets/filelistwidget.h"
#include "widgets/filterlistwidget.h"
#include "widgets/searchlistwidget.h"
#include "widgets/outputdisplaywidget.h"
#include "../core/Logger.h"
#include "../core/TimeUtils.h"
#include "mainwindow.h"
#include "../core/TroubleshootingLogger.h"
#include "../bridge/FilterAdapter.h"
#include <QFileInfo>
#include <memory>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include "../core/FileSystem.h"
#include "../bridge/QtBridge.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

// Forward declaration for the saveWorkspaces method
class MainWindow;

Workspace::Workspace(int64_t id, QWidget *parent)
    : QWidget(parent), bridge(QtBridge::getInstance()), workspaceId(id)
{    
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Create name label
    //nameLabel = new QLabel(workspaceName, this);
    //nameLabel->setStyleSheet("font-weight: bold;");
    //layout->addWidget(nameLabel);
    
    // Create splitter
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    layout->addWidget(splitter);

    // Top section: file list, filter list, search list
    QWidget *topWidget = new QWidget;  // No parent, let splitter manage
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);

    fileListWidget = new FileListWidget(workspaceId ,bridge, topWidget);
    filterListWidget = new FilterListWidget(workspaceId, bridge, topWidget);
    searchListWidget = new SearchListWidget(workspaceId, bridge, topWidget);

    // Create horizontal splitters for the top section
    QSplitter *leftSplitter = new QSplitter(Qt::Horizontal, topWidget);
    topLayout->addWidget(leftSplitter);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // Add the file list widget directly to the first splitter
    leftSplitter->addWidget(fileListWidget);

    // Create a second splitter for filter and search lists
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, leftSplitter);
    leftSplitter->addWidget(rightSplitter);
    
    // Add filter and search widgets to the second splitter
    rightSplitter->addWidget(filterListWidget);
    rightSplitter->addWidget(searchListWidget);

    // Set stretch factors for the splitters
    leftSplitter->setStretchFactor(0, 1); // fileListWidget
    leftSplitter->setStretchFactor(1, 2); // rightSplitter (contains filter and search)
    
    rightSplitter->setStretchFactor(0, 1); // filterListWidget
    rightSplitter->setStretchFactor(1, 1); // searchListWidget

    // Bottom section: output display area
    outputDisplay = new OutputDisplayWidget(workspaceId, bridge, topWidget);  // No parent, let splitter manage

    // Add top and bottom sections to splitter
    splitter->addWidget(topWidget);
    splitter->addWidget(outputDisplay);

    // Set initial proportions
    splitter->setSizes(QList<int>() << 200 << 400);
    
    // Connect file selection signals
    connect(fileListWidget, &FileListWidget::filesChanged, this, &Workspace::onFilesChanged);
        
    // Connect filter change signal
    connect(filterListWidget, &FilterListWidget::filtersChanged, this, &Workspace::onFiltersChanged); 
    connect(filterListWidget, &FilterListWidget::navigateToNextMatch, outputDisplay, &OutputDisplayWidget::onNavigateToNextFilterMatch);
    connect(filterListWidget, &FilterListWidget::navigateToPreviousMatch, outputDisplay, &OutputDisplayWidget::onNavigateToPreviousFilterMatch);

    // Connect search change signal
    connect(searchListWidget, &SearchListWidget::searchsChanged, this, &Workspace::onSearchsChanged);
    connect(searchListWidget, &SearchListWidget::navigateToNextMatch, outputDisplay, &OutputDisplayWidget::onNavigateToNextSearchMatch);
    connect(searchListWidget, &SearchListWidget::navigateToPreviousMatch, outputDisplay, &OutputDisplayWidget::onNavigateToPreviousSearchMatch);
        
    bridge.logInfo("Workspace Created workspace: " + QString::number(workspaceId));
}


Workspace::~Workspace()
{
    // Destructor
}


void Workspace::initializeWithData()
{
    this->workspaceName = bridge.getWorkspaceName(workspaceId);
    fileListWidget->initializeWithData(workspaceId);
    filterListWidget->initializeWithData(workspaceId);
    searchListWidget->initializeWithData(workspaceId);
    
    // Set workspace name from bridge
    QString name = bridge.getWorkspaceName(workspaceId);
    if (!name.isEmpty()) {
        workspaceName = name;
        //nameLabel->setText(name);
    }
}

void Workspace::updateTranslation()
{
}

QString Workspace::getDisplayName() const
{
    return workspaceName;
}

bool Workspace::setWorkspaceName(const QString &name)
{
    // Update the workspace name in the bridge if we have a valid index
    if (workspaceId >= 0) {
        if (bridge.setWorkspaceName(workspaceId, name)) {
            // Get the actual name that was set (might be different if a unique name was generated)
            QString actualName = bridge.getWorkspaceName(workspaceId);
            workspaceName = actualName;
            //nameLabel->setText(actualName);            
            return true;
        }
        return false;
    }
    
    // If we don't have a valid index yet, just update the UI
    workspaceName = name;
    //nameLabel->setText(name);
    return true;
}

int Workspace::getSortIndex() const
{
    return bridge.getWorkspaceSortIndex(workspaceId);
}

void Workspace::setSortIndex(int index)
{
    bridge.setWorkspaceSortIndex(workspaceId, index);
}

void Workspace::stopLoading()
{
}

QStringList Workspace::getSelectedFiles() const
{
    QList<FileInfo> selectedFiles = fileListWidget->getSelectedFiles();
    QStringList filePaths;
    for (const FileInfo &fileInfo : selectedFiles) {
        filePaths.append(fileInfo.filePath);
    }
    return filePaths;
}

void Workspace::addSearch(const QString &query, bool caseSensitive, bool wholeWord, bool regex)
{
    // Create SearchConfig object
    SearchConfig searchConfig;
    searchConfig.searchPattern = query;
    searchConfig.caseSensitive = caseSensitive;
    searchConfig.wholeWord = wholeWord;
    searchConfig.isRegex = regex;
    
    // Add search to the bridge if we have a valid workspace index
    if (workspaceId >= 0) {
        if (bridge.addSearchToWorkspace(workspaceId, searchConfig)) {
            // Add search to UI
            searchListWidget->addSearch(searchConfig);
            bridge.logInfo("Workspace Added search to workspace: " + query);
        } else {
            bridge.logError("Failed to add search to workspace: " + query);
        }
    } else {
        // Just add to UI if we don't have a valid workspace index yet
        searchListWidget->addSearch(searchConfig);
        bridge.logInfo("Workspace Added search to workspace UI: " + query);
    }
}

void Workspace::saveWorkspaceData()
{
    if (workspaceId >= 0) {
        // The bridge will handle saving the workspace data
        bridge.logInfo("Workspace Workspace data modified, will be saved on application close");
    }
}

void Workspace::setActive(bool active)
{
    outputDisplay->doUpdate();
    filterListWidget->doUpdate();
    searchListWidget->doUpdate();
}

void Workspace::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Any initialization that needs to happen when the workspace becomes visible
}

void Workspace::onFilesChanged()
{
    outputDisplay->doUpdate();
    filterListWidget->doUpdate();
    searchListWidget->doUpdate();
}

void Workspace::onRequestFileIndex(const QString &filePath, int &fileIndex)
{
    fileIndex = fileListWidget->findFileIndex(filePath);
}

void Workspace::onFiltersChanged()
{
    outputDisplay->doUpdate();
    filterListWidget->doUpdate();
    searchListWidget->doUpdate();
}

void Workspace::onSearchsChanged()
{
    outputDisplay->doUpdate();
    filterListWidget->doUpdate();
    searchListWidget->doUpdate();
}

void Workspace::onFilterMatchCountsUpdated(const QMap<int, int> &matchCounts)
{
}


