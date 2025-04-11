#include "mainwindow.h"
#include "workspace.h"
#include <QMenuBar>
#include <QToolBar>
#include <QLabel>
#include <QApplication>
#include <QCloseEvent>
#include <QActionGroup>
#include <QMouseEvent>
#include <QTabWidget>
#include <QTabBar>
#include <QInputDialog>
#include <QTextStream>
#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QMenu>
#include "../bridge/QtBridge.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings("LogTools", "TxtLogParser"), bridge(QtBridge::getInstance())
{
    bridge.logInfo("MainWindow MainWindow::MainWindow Enter");
    
    tabWidget = new QTabWidget(this);
    tabWidget->setTabPosition(QTabWidget::North);  // Ensure tabs are at the top
    tabWidget->setElideMode(Qt::ElideRight);      // Elide text on the right if needed
    tabWidget->setUsesScrollButtons(true);         // Enable scrolling if many tabs
    tabWidget->setMovable(true);                  // Allow tab reordering
    tabWidget->setDocumentMode(true);  // Make tabs look more modern
    tabWidget->setTabsClosable(true);  // Allow tabs to be closed
    
    // Style the tab widget with high contrast active/inactive states
    QString tabStyle = R"(
        QTabBar::tab {
            background-color: #f0f0f0;
            border: 1px solid #c0c0c0;
            padding: 6px 12px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color:rgb(76, 93, 149);
            border-bottom-color: #ffffff;
        }
        QTabBar::tab:!selected {
            margin-top: 2px;
        }
    )";
    tabWidget->setStyleSheet(tabStyle);
    
    setCentralWidget(tabWidget);
    
    // Connect tab close signal
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeWorkspaceAtIndex);
    
    // Connect tab moved signal to handle drag and drop reordering
    connect(tabWidget->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
        bridge.logInfo("MainWindow Tab moved from " + QString::number(from) + " to " + QString::number(to));
        // Update the workspace order in the bridge if needed
        // This ensures the workspace order is maintained when tabs are reordered
        updateWorkspaceSortIndex();
    });
    
    // Connect tab change signal
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            Workspace* workspace = qobject_cast<Workspace*>(tabWidget->widget(index));
            if (workspace) {
                int64_t workspaceId = workspace->getWorkspaceId();
                bridge.logInfo("MainWindow Switching to workspace index: " + QString::number(index) + " id: " + QString::number(workspaceId));
                bridge.setActiveWorkspace(workspaceId);
                workspace->setActive(true);
            }
        }
    });
    
    // Create menus and toolbars
    createMenus();
    createToolBars();
    createLanguageMenu();
    
    // Install event filter on tabWidget's tab bar
    tabWidget->tabBar()->installEventFilter(this);
    
    // Restore window geometry before showing
    restoreWindowGeometry();
    
    // Show UI
    show();
    QApplication::processEvents();
    
    // Use a single-shot timer to load workspaces after UI is shown
    QTimer::singleShot(0, this, &MainWindow::loadWorkspaces);
    
    bridge.logInfo("MainWindow MainWindow::MainWindow Exit");
}

MainWindow::~MainWindow() {
    // Destructor
}

void MainWindow::createMenus() {
    // Create workspace menu
    workspaceMenu = menuBar()->addMenu(tr("Workspace"));
    
    // 设置菜单样式，包括鼠标悬停效果
    workspaceMenu->setStyleSheet(R"(
        QMenu {
            background-color: palette(window);
            color: palette(text);
            border: 1px solid palette(mid);
            padding: 5px;
        }
        QMenu::item {
            padding: 6px 20px;
            border-radius: 3px;
            margin: 2px;
        }
        QMenu::item:selected {
            background-color: palette(highlight);
            color: palette(highlighted-text);
        }
        QMenu::item:hover {
            background-color: palette(mid);
            color: palette(text);
        }
    )");
    
    // Create workspace action group for exclusive selection
    workspaceActionGroup = new QActionGroup(this);
    workspaceActionGroup->setExclusive(true);
    
    // Create new workspace action
    newWorkspaceAction = new QAction(tr("New Workspace"), this);
    connect(newWorkspaceAction, &QAction::triggered, this, &MainWindow::createWorkspace);
    workspaceMenu->addAction(newWorkspaceAction);
    
    // Create close workspace action
    closeWorkspaceAction = new QAction(tr("Close Workspace"), this);
    connect(closeWorkspaceAction, &QAction::triggered, this, &MainWindow::closeWorkspace);
    workspaceMenu->addAction(closeWorkspaceAction);
    
    // Add language menu
    languageMenu = menuBar()->addMenu(tr("Language"));
    
    // 设置语言菜单样式，包括鼠标悬停效果
    languageMenu->setStyleSheet(R"(
        QMenu {
            background-color: palette(window);
            color: palette(text);
            border: 1px solid palette(mid);
            padding: 5px;
        }
        QMenu::item {
            padding: 6px 20px;
            border-radius: 3px;
            margin: 2px;
        }
        QMenu::item:selected {
            background-color: palette(highlight);
            color: palette(highlighted-text);
        }
        QMenu::item:hover {
            background-color: palette(mid);
            color: palette(text);
        }
    )");
}

void MainWindow::createToolBars() {
    QToolBar *toolbar = addToolBar(tr("Workspace"));
    toolbar->addAction(newWorkspaceAction);
}

void MainWindow::createLanguageMenu() {
    QAction *chineseAction = new QAction(tr("Chinese"), this);
    QAction *englishAction = new QAction(tr("English"), this);
    
    connect(chineseAction, &QAction::triggered, this, &MainWindow::switchToChinese);
    connect(englishAction, &QAction::triggered, this, &MainWindow::switchToEnglish);
    
    languageMenu->addAction(chineseAction);
    languageMenu->addAction(englishAction);
}

void MainWindow::switchToChinese() {
    // Load Chinese translation
    if (translator.load(":/translations/workspace_zh_CN.qm")) {
        QCoreApplication::installTranslator(&translator);
        retranslateUi();
        bridge.logInfo("MainWindow Switched to Chinese language");
    } else {
        bridge.logError("Failed to load Chinese translation");
    }
}

void MainWindow::switchToEnglish() {
    // Remove translator to revert to English
    QCoreApplication::removeTranslator(&translator);
    retranslateUi();
    bridge.logInfo("MainWindow Switched to English language");
}

void MainWindow::retranslateUi() {
    // Update UI text
    workspaceMenu->setTitle(tr("Workspace"));
    newWorkspaceAction->setText(tr("New Workspace"));
    closeWorkspaceAction->setText(tr("Close Workspace"));
    languageMenu->setTitle(tr("Language"));
    
    // Update workspace tabs
    for (int i = 0; i < tabWidget->count(); ++i) {
        Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            workspace->updateTranslation();
        }
    }
}

void MainWindow::createWorkspace() {
    // Let the bridge generate the workspace name
    int64_t workspaceId = bridge.createWorkspace();
    
    // Create workspace UI
    Workspace *workspace = new Workspace(workspaceId, this);
    workspace->initializeWithData();
    
    // Add to tab widget
    QString displayName = workspace->getDisplayName();
    int tabIndex = tabWidget->addTab(workspace, displayName);
    workspace->setSortIndex(tabIndex);
    tabWidget->setCurrentIndex(tabIndex);
    
    // Update workspace menu
    updateWorkspaceMenu();
    
    bridge.logInfo("MainWindow Created workspace: " + displayName);
}

void MainWindow::closeWorkspace() {
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex >= 0) {
        closeWorkspaceAtIndex(currentIndex);
    }
}

void MainWindow::closeWorkspaceAtIndex(int index) {
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }
    
    // Get workspace name for logging
    Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(index));
    QString name = workspace ? workspace->getDisplayName() : QString::number(index);
    
    int64_t workspaceId = workspace->getWorkspaceId();
    // Remove workspace from bridge
    bridge.removeWorkspace(workspaceId);
    
    // Remove tab
    tabWidget->removeTab(index);
    delete workspace;
    
    // Update workspace menu
    updateWorkspaceMenu();
    updateWorkspaceSortIndex();
    
    bridge.logInfo("MainWindow Closed workspace: " + name);
}

void MainWindow::updateWorkspaceMenu() {
    bridge.logInfo("MainWindow Updating workspace menu");
    // Clear existing actions
    for (QAction* action : workspaceActions) {
        workspaceMenu->removeAction(action);
    }
    workspaceActions.clear();
    
    bridge.logInfo("MainWindow After Removed existing workspace actions");
    // Add actions for each workspace
    for (int i = 0; i < tabWidget->count(); ++i) {
        Workspace* workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            QAction* action = workspaceMenu->addAction(workspace->getDisplayName());
            action->setCheckable(true);
            action->setChecked(tabWidget->currentIndex() == i);
            action->setData(i); // Store the tab index in the action's data
            workspaceActions.append(action);
            workspaceActionGroup->addAction(action);
            
            // Connect the action to switch to this workspace
            connect(action, &QAction::triggered, this, [this, i]() {
                if (i >= 0 && i < tabWidget->count()) {
                    tabWidget->setCurrentIndex(i);
                }
            });
        }
    }
    
    // Add a separator
    workspaceMenu->addSeparator();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == tabWidget->tabBar()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton) {
                int tabIndex = tabWidget->tabBar()->tabAt(mouseEvent->pos());
                if (tabIndex >= 0) {
                    bridge.logInfo("MainWindow Right-clicked on tab " + QString::number(tabIndex));
                    
                    QMenu contextMenu(this);
                    // 设置菜单样式，包括鼠标悬停效果
                    contextMenu.setStyleSheet(R"(
                        QMenu {
                            background-color: palette(window);
                            color: palette(text);
                            border: 1px solid palette(mid);
                            padding: 5px;
                        }
                        QMenu::item {
                            padding: 6px 20px;
                            border-radius: 3px;
                            margin: 2px;
                        }
                        QMenu::item:selected {
                            background-color: palette(highlight);
                            color: palette(highlighted-text);
                        }
                        QMenu::item:hover {
                            background-color: palette(mid);
                            color: palette(text);
                        }
                    )");
                    
                    // Add rename action
                    QAction* renameAction = contextMenu.addAction(tr("Rename"));
                    connect(renameAction, &QAction::triggered, this, [this, tabIndex]() {
                        if (Workspace* ws = qobject_cast<Workspace*>(tabWidget->widget(tabIndex))) {
                            QString oldName = ws->getDisplayName();
                            bridge.logInfo("MainWindow Opening rename dialog for workspace '" + oldName + "' at index " + QString::number(tabIndex));
                            
                            QString newName = QInputDialog::getText(this, 
                                tr("Rename Workspace"),
                                tr("Enter new name:"),
                                QLineEdit::Normal,
                                oldName);
                            
                            if (!newName.isEmpty() && newName != oldName) {
                                bridge.logInfo("MainWindow Attempting to rename workspace from '" + oldName + "' to '" + newName + "'");
                                
                                if (ws->setWorkspaceName(newName)) {
                                    tabWidget->setTabText(tabIndex, newName);
                                    updateWorkspaceMenu();
                                    bridge.logInfo("MainWindow Successfully renamed workspace from '" + oldName + "' to '" + newName + "'");
                                } else {
                                    bridge.logError("Failed to rename workspace '" + oldName + "' to '" + newName + "'");
                                    QMessageBox::warning(this,
                                        tr("Rename Failed"),
                                        tr("Failed to rename workspace. The name might be invalid or already in use."),
                                        QMessageBox::Ok);
                                }
                            } else {
                                bridge.logInfo("MainWindow Workspace rename cancelled or unchanged");
                            }
                        }
                    });
                    
                    // Add close action
                    QAction* closeAction = contextMenu.addAction(tr("Close"));
                    connect(closeAction, &QAction::triggered, this, [this, tabIndex]() {
                        bridge.logInfo("MainWindow Closing workspace at index " + QString::number(tabIndex));
                        closeWorkspaceAtIndex(tabIndex);
                    });
                    
                    // Show context menu at cursor position
                    contextMenu.exec(mouseEvent->globalPosition().toPoint());
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    bridge.logInfo("MainWindow === Application closing ===");
    
    // Save workspaces and settings before closing
    saveWorkspaces();
    saveSettings();
    saveWindowGeometry();
    
    bridge.logInfo("MainWindow === Application closed ===");
    event->accept();
}

void MainWindow::saveSettings() {
    settings.setValue("language", "");
    bridge.logInfo("MainWindow Saved settings");
}

void MainWindow::loadSettings() {
    restoreWindowGeometry();
    bridge.logInfo("MainWindow Loaded settings");
}

void MainWindow::saveWindowGeometry() {
    QByteArray geometry = saveGeometry();
    QByteArray state = saveState();
    settings.setValue("geometry", geometry);
    settings.setValue("windowState", state);
    bridge.logInfo("MainWindow Saved window geometry: " + QString::number(pos().x()) + "," + QString::number(pos().y()) + 
                  " size: " + QString::number(size().width()) + "x" + QString::number(size().height()));
}

void MainWindow::restoreWindowGeometry() {
    if (settings.contains("geometry")) {
        QByteArray geometry = settings.value("geometry").toByteArray();
        if (restoreGeometry(geometry)) {
            bridge.logInfo("MainWindow Restored window geometry: " + QString::number(pos().x()) + "," + QString::number(pos().y()) + 
                          " size: " + QString::number(size().width()) + "x" + QString::number(size().height()));
        } else {
            bridge.logWarning("Failed to restore window geometry");
        }
    } else {
        bridge.logInfo("MainWindow No saved window geometry found, using default");
    }
    
    if (settings.contains("windowState")) {
        QByteArray state = settings.value("windowState").toByteArray();
        if (restoreState(state)) {
            bridge.logInfo("MainWindow Restored window state");
        } else {
            bridge.logWarning("Failed to restore window state");
        }
    }
}

void MainWindow::loadWorkspaces() {
    bridge.logInfo("Enter MainWindow::loadWorkspaces");
    
    // Load workspaces through bridge
    if (bridge.loadWorkspaces()) {
        // Create workspace UI for each workspace
        QVector<int64_t> workspaceIds = bridge.getAllWorkspaceIds();
        int64_t activeWorkspaceId = bridge.getActiveWorkspace();
        bridge.logInfo("MainWindow::loadWorkspaces Successfully loaded " + QString::number(workspaceIds.size()) + " workspaces from configuration " +
                      "with active id " + QString::number(activeWorkspaceId));
        // Set active workspace
        int activeIndex = -1;
        for (int i = 0; i < workspaceIds.size(); i++) {
            int64_t workspaceId = workspaceIds[i];
            if (workspaceId == activeWorkspaceId) {
                activeIndex = i;
            }
            bridge.logInfo("MainWindow::loadWorkspaces Creating workspace for index " + QString::number(workspaceId));
            Workspace *workspace = new Workspace(workspaceId, this);
            bridge.logInfo("MainWindow::loadWorkspaces before initializeWithData index " + QString::number(workspaceId));
            workspace->initializeWithData();
            
            // Add to tab widget
            QString displayName = workspace->getDisplayName();
            int tabIndex = tabWidget->addTab(workspace, displayName);
            bridge.logInfo("MainWindow::loadWorkspaces Added workspace '" + displayName + "' at index " + QString::number(tabIndex));
        }
        bridge.logInfo("MainWindow Created all workspaces");
        if (activeIndex >= 0 && activeIndex < tabWidget->count()) {
            tabWidget->setCurrentIndex(activeIndex);
            bridge.logInfo("MainWindow::loadWorkspaces Set active workspace to index " + QString::number(activeIndex));
        } else {
            bridge.logWarning("MainWindow::loadWorkspaces active workspace index: " + QString::number(activeIndex));
        }
        
        // Update workspace menu
        updateWorkspaceMenu();
        bridge.logInfo("MainWindow::loadWorkspaces Updated workspace menu");
        
        bridge.logInfo("MainWindow::loadWorkspaces Successfully loaded and initialized ");
    } else {
        bridge.logWarning("MainWindow::loadWorkspaces Failed to load workspaces from configuration");
        
        // Create default workspace if no workspaces were loaded
        if (tabWidget->count() == 0) {
            bridge.logInfo("MainWindow::loadWorkspaces Creating default workspace");
            createWorkspace();
        }
    }
    
    bridge.logInfo("Leave MainWindow::loadWorkspaces");
}

void MainWindow::saveWorkspaces() {
    bridge.logInfo("MainWindow === Starting workspace saving ===");
    bridge.logInfo("MainWindow Current workspace count: " + QString::number(tabWidget->count()));
    
    // Save workspaces through bridge
    if (bridge.saveWorkspaces()) {
        bridge.logInfo("MainWindow Successfully saved workspaces to configuration");        
    } else {
        bridge.logError("Failed to save workspaces to configuration");
    }
    
    bridge.logInfo("MainWindow === Workspace saving completed ===");
}

void MainWindow::updateWorkspaceSortIndex() {
    bridge.logInfo("Enter MainWindow::updateWorkspaceSortIndex");
    
    // Collect workspace IDs in the current tab order
    QVector<int64_t> orderedIds;
    for (int i = 0; i < tabWidget->count(); ++i) {
        Workspace* workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            int oldIndex = workspace->getSortIndex();
            if(oldIndex != i) {
                workspace->setSortIndex(i);
                bridge.logInfo("MainWindow::updateWorkspaceSortIndex workspace " + QString::number(workspace->getWorkspaceId()) + " moved from " + QString::number(oldIndex) + " to " + QString::number(i));
            }
        }
    }
}

void MainWindow::updateWorkspaceNames() {
    // Update tab names
    for (int i = 0; i < tabWidget->count(); ++i) {
        Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            tabWidget->setTabText(i, workspace->getDisplayName());
        }
    }
    
    // Update workspace menu
    updateWorkspaceMenu();
}

void MainWindow::renameWorkspace()
{
    QMenu *menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) {
        bridge.logError("Failed to rename workspace: Invalid menu context");
        return;
    }
    
    QAction *action = menu->property("actionSender").value<QAction*>();
    if (!action) {
        bridge.logError("Failed to rename workspace: Invalid action");
        return;
    }
    
    int index = action->data().toInt();
    if (index < 0 || index >= tabWidget->count()) {
        bridge.logError("Failed to rename workspace: Invalid index " + QString::number(index));
        return;
    }
    
    Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(index));
    if (!workspace) {
        bridge.logError("Failed to rename workspace: Invalid workspace at index " + QString::number(index));
        return;
    }
    
    QString oldName = workspace->getDisplayName();
    bridge.logInfo("MainWindow Opening rename dialog for workspace '" + oldName + "' at index " + QString::number(index));
    
    bool ok;
    QString newName = QInputDialog::getText(this, 
                                          tr("Rename Workspace"),
                                          tr("Enter new name:"), 
                                          QLineEdit::Normal,
                                          oldName,
                                          &ok);
    
    if (!ok) {
        bridge.logInfo("MainWindow Workspace rename cancelled by user");
        return;
    }
    
    if (newName == oldName) {
        bridge.logInfo("MainWindow Workspace name unchanged");
        return;
    }
    
    bridge.logInfo("MainWindow Attempting to rename workspace from '" + oldName + "' to '" + newName + "'");
    
    // Try to set the new name
    if (workspace->setWorkspaceName(newName)) {
        // Update UI elements
        tabWidget->setTabText(index, newName);
        action->setText(newName);
        updateWorkspaceMenu();
        
        bridge.logInfo("MainWindow Successfully renamed workspace from '" + oldName + "' to '" + newName + "'");
    } else {
        bridge.logError("Failed to rename workspace '" + oldName + "' to '" + newName + "'");
        QMessageBox::warning(this,
                           tr("Rename Failed"),
                           tr("Failed to rename workspace. The name might be invalid or already in use."),
                           QMessageBox::Ok);
    }
}

void MainWindow::closeWorkspaceByAction()
{
    QMenu *menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    
    QAction *action = menu->property("actionSender").value<QAction*>();
    if (!action) return;
    
    int index = action->data().toInt();
    closeWorkspaceAtIndex(index);
}

void MainWindow::showWorkspaceContextMenu(QAction* actionSender)
{
    QMenu contextMenu(this);
    
    // 设置菜单样式，包括鼠标悬停效果
    contextMenu.setStyleSheet(R"(
        QMenu {
            background-color: palette(window);
            color: palette(text);
            border: 1px solid palette(mid);
            padding: 5px;
        }
        QMenu::item {
            padding: 6px 20px;
            border-radius: 3px;
            margin: 2px;
        }
        QMenu::item:selected {
            background-color: palette(highlight);
            color: palette(highlighted-text);
        }
        QMenu::item:hover {
            background-color: palette(mid);
            color: palette(text);
        }
    )");
    
    QAction *renameAction = new QAction(tr("Rename Current Workspace"), this);
    QAction *closeAction = new QAction(tr("Close Current Workspace"), this);
    
    connect(renameAction, &QAction::triggered, this, &MainWindow::renameWorkspace);
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeWorkspaceByAction);
    
    contextMenu.addAction(renameAction);
    contextMenu.addAction(closeAction);
    
    // Store the sender action for use in the slots
    contextMenu.setProperty("actionSender", QVariant::fromValue(actionSender));
    
    contextMenu.exec(QCursor::pos());
}
