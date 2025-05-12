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
#include "StyleManager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings("paneltree", "TxtLogParser"), bridge(QtBridge::getInstance()), plusTabIndex(-1)
{
    bridge.logInfo("[MainWindow] MainWindow::MainWindow Enter");
    
    tabWidget = new QTabWidget(this);
    tabWidget->setTabPosition(QTabWidget::North);  // Ensure tabs are at the top
    tabWidget->setElideMode(Qt::ElideRight);      // Elide text on the right if needed
    tabWidget->setUsesScrollButtons(true);         // Enable scrolling if many tabs
    tabWidget->setMovable(true);                  // Allow tab reordering
    tabWidget->setDocumentMode(true);  // Make tabs look more modern
    tabWidget->setTabsClosable(true);  // Allow tabs to be closed
    tabWidget->setStyleSheet(StyleManager::instance().getTabStyle());
    
    setCentralWidget(tabWidget);
    
    // Connect tab close signal with filter for "+" tab
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        // Skip close request if it's the "+" tab
        if (index != plusTabIndex) {
            closeWorkspaceAtIndex(index);
        }
    });
    
    // Connect tab moved signal to handle drag and drop reordering
    connect(tabWidget->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
        bridge.logInfo("[MainWindow] Tab moved from " + QString::number(from) + " to " + QString::number(to));
        // Update the workspace order in the bridge if needed
        // This ensures the workspace order is maintained when tabs are reordered
        updateWorkspaceSortIndex();
    });
    
    // Connect tab change signal
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(tabWidget, &QTabWidget::tabBarClicked, this, &MainWindow::onTabClicked);
    
    // Create menus and toolbars
    createMenus();
    //createLanguageMenu();
    
    // Install event filter on tabWidget's tab bar
    tabWidget->tabBar()->installEventFilter(this);
    
    // Restore window geometry before showing
    restoreWindowGeometry();
    
    // Show UI
    show();
    QApplication::processEvents();
    
    // Use a single-shot timer to load workspaces after UI is shown
    QTimer::singleShot(0, this, &MainWindow::loadWorkspaces);
    
    bridge.logInfo("[MainWindow] MainWindow::MainWindow Exit");
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
    
    // Add Help menu
    helpMenu = menuBar()->addMenu(tr("Help"));
    
    // 设置Help菜单样式，与其他菜单保持一致
    helpMenu->setStyleSheet(R"(
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
    
    // Create About action
    aboutAction = new QAction(tr("About"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    helpMenu->addAction(aboutAction);
    
    // disable language menu for now
    /*
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
    */
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
        bridge.logInfo("[MainWindow] Switched to Chinese language");
    } else {
        bridge.logError("[MainWindow] Failed to load Chinese translation");
    }
}

void MainWindow::switchToEnglish() {
    // Remove translator to revert to English
    QCoreApplication::removeTranslator(&translator);
    retranslateUi();
    bridge.logInfo("[MainWindow] Switched to English language");
}

void MainWindow::retranslateUi() {
    // Update UI text
    workspaceMenu->setTitle(tr("Workspace"));
    newWorkspaceAction->setText(tr("New Workspace"));
    closeWorkspaceAction->setText(tr("Close Workspace"));
    languageMenu->setTitle(tr("Language"));
    
    // Update workspace tabs
    for (int i = 0; i < tabWidget->count(); ++i) {
        if(i == plusTabIndex) {
            continue; // Skip the "+" tab
        }
        Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            workspace->updateTranslation();
        }
    }
}

void MainWindow::createWorkspace() {
    bridge.logInfo("[MainWindow] Enter MainWindow::createWorkspace");
    // Remove the plus tab temporarily
    if (plusTabIndex >= 0) {
        QWidget* oldWidget = tabWidget->widget(plusTabIndex);
        tabWidget->removeTab(plusTabIndex);
        delete oldWidget;  // Explicitly delete the widget
        plusTabIndex = -1;
    }
    
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
    
    // Add the plus tab back at the end
    addPlusTab();
    
    // Update workspace menu
    updateWorkspaceMenu();
    
    bridge.logInfo("[MainWindow] Leave MainWindow::createdWorkspace " + displayName);
}

void MainWindow::closeWorkspace() {
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex >= 0) {
        closeWorkspaceAtIndex(currentIndex);
    }
}

void MainWindow::closeWorkspaceAtIndex(int index) {
    bridge.logInfo("[MainWindow] Enter MainWindow::closedWorkspace: " + QString::number(index));
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }
    // Skip if it's the "+" tab
    if (index == plusTabIndex) {
        return;
    }
    if(plusTabIndex > index) {
        QWidget* oldWidget = tabWidget->widget(plusTabIndex);
        tabWidget->removeTab(plusTabIndex);
        delete oldWidget;  // Explicitly delete the widget
        plusTabIndex = -1;
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
    
    addPlusTab();
    updateWorkspaceMenu();
    updateWorkspaceSortIndex();
    
    bridge.logInfo("[MainWindow] Leave MainWindow::closedWorkspace: " + QString::number(index) + ",name:" + name);
}

void MainWindow::updateWorkspaceMenu() {
    bridge.logInfo("[MainWindow] Enter MainWindow::updateWorkspaceMenu");
    // Clear existing actions
    for (QAction* action : workspaceActions) {
        workspaceMenu->removeAction(action);
    }
    workspaceActions.clear();
    
    bridge.logInfo("MainWindow After Removed existing workspace actions");
    // Add actions for each workspace
    for (int i = 0; i < tabWidget->count(); ++i) {
        if(i == plusTabIndex) {
            continue; // Skip the "+" tab
        }
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
                    if (i == plusTabIndex) {
                        return; // Skip the "+" tab
                    }
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
                    // Skip showing context menu if this is the "+" tab
                    if (tabIndex == plusTabIndex) {
                        return true; // Process the event and prevent the default context menu
                    }
                    
                    bridge.logInfo("[MainWindow] Right-clicked on tab " + QString::number(tabIndex));
                    
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
                            bridge.logInfo("[MainWindow] Opening rename dialog for workspace '" + oldName + "' at index " + QString::number(tabIndex));
                            
                            QString newName = QInputDialog::getText(this, 
                                tr("Rename Workspace"),
                                tr("Enter new name:"),
                                QLineEdit::Normal,
                                oldName);
                            
                            if (!newName.isEmpty() && newName != oldName) {
                                bridge.logInfo("[MainWindow] Attempting to rename workspace from '" + oldName + "' to '" + newName + "'");
                                
                                if (ws->setWorkspaceName(newName)) {
                                    tabWidget->setTabText(tabIndex, newName);
                                    updateWorkspaceMenu();
                                    bridge.logInfo("[MainWindow] Successfully renamed workspace from '" + oldName + "' to '" + newName + "'");
                                } else {
                                    bridge.logError("Failed to rename workspace '" + oldName + "' to '" + newName + "'");
                                    QMessageBox::warning(this,
                                        tr("Rename Failed"),
                                        tr("Failed to rename workspace. The name might be invalid or already in use."),
                                        QMessageBox::Ok);
                                }
                            } else {
                                bridge.logInfo("[MainWindow] Workspace rename cancelled or unchanged");
                            }
                        }
                    });
                    
                    // Add close action
                    QAction* closeAction = contextMenu.addAction(tr("Close"));
                    connect(closeAction, &QAction::triggered, this, [this, tabIndex]() {
                        bridge.logInfo("[MainWindow] Closing workspace at index " + QString::number(tabIndex));
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
    bridge.logInfo("[MainWindow] === Application closing ===");
    
    // Save workspaces and settings before closing
    saveWorkspaces();
    saveSettings();
    saveWindowGeometry();
    
    bridge.logInfo("[MainWindow] === Application closed ===");
    event->accept();
}

void MainWindow::saveSettings() {
    settings.setValue("language", "");
    bridge.logInfo("[MainWindow] Saved settings");
}

void MainWindow::loadSettings() {
    restoreWindowGeometry();
    bridge.logInfo("[MainWindow] Loaded settings");
}

void MainWindow::saveWindowGeometry() {
    QByteArray geometry = saveGeometry();
    QByteArray state = saveState();
    settings.setValue("geometry", geometry);
    settings.setValue("windowState", state);
    bridge.logInfo("[MainWindow] Saved window geometry: " + QString::number(pos().x()) + "," + QString::number(pos().y()) + 
                  " size: " + QString::number(size().width()) + "x" + QString::number(size().height()));
}

void MainWindow::restoreWindowGeometry() {
    if (settings.contains("geometry")) {
        QByteArray geometry = settings.value("geometry").toByteArray();
        if (restoreGeometry(geometry)) {
            bridge.logInfo("[MainWindow] Restored window geometry: " + QString::number(pos().x()) + "," + QString::number(pos().y()) + 
                          " size: " + QString::number(size().width()) + "x" + QString::number(size().height()));
        } else {
            bridge.logWarning("Failed to restore window geometry");
        }
    } else {
        bridge.logInfo("[MainWindow] No saved window geometry found, using default");
        
        // Set default window geometry for first launch
        // Get the available screen geometry
        QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
        int defaultWidth = qMin(1024, screenGeometry.width() - 100);
        int defaultHeight = qMin(768, screenGeometry.height() - 100);
        
        // Center the window on screen
        int x = (screenGeometry.width() - defaultWidth) / 2;
        int y = (screenGeometry.height() - defaultHeight) / 2;
        
        // Set window geometry
        setGeometry(x, y, defaultWidth, defaultHeight);
        
        bridge.logInfo("[MainWindow] Set default window geometry: " + QString::number(x) + "," + QString::number(y) + 
                      " size: " + QString::number(defaultWidth) + "x" + QString::number(defaultHeight));
    }
    
    if (settings.contains("windowState")) {
        QByteArray state = settings.value("windowState").toByteArray();
        if (restoreState(state)) {
            bridge.logInfo("[MainWindow] Restored window state");
        } else {
            bridge.logWarning("Failed to restore window state");
        }
    } else {
        // Set default window state for first launch
        // This will ensure toolbars and docks are in the default positions
        setWindowState(Qt::WindowActive);
        bridge.logInfo("[MainWindow] Set default window state to active");
    }
}

void MainWindow::loadWorkspaces() {
    bridge.logInfo("[MainWindow] Enter MainWindow::loadWorkspaces");
    
    // Load workspaces through bridge
    if (bridge.loadWorkspaces()) {
        // Create workspace UI for each workspace
        QVector<int64_t> workspaceIds = bridge.getAllWorkspaceIds();
        int64_t activeWorkspaceId = bridge.getActiveWorkspace();
        bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Successfully loaded " + QString::number(workspaceIds.size()) + " workspaces from configuration " +
                      "with active id " + QString::number(activeWorkspaceId));
        // Set active workspace
        int activeIndex = -1;
        for (int i = 0; i < workspaceIds.size(); i++) {
            int64_t workspaceId = workspaceIds[i];
            if (workspaceId == activeWorkspaceId) {
                activeIndex = i;
            }
            bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Creating workspace for index " + QString::number(workspaceId));
            Workspace *workspace = new Workspace(workspaceId, this);
            bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces before initializeWithData index " + QString::number(workspaceId));
            workspace->initializeWithData();
            
            // Add to tab widget
            QString displayName = workspace->getDisplayName();
            int tabIndex = tabWidget->addTab(workspace, displayName);
            bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Added workspace '" + displayName + "' at index " + QString::number(tabIndex));
        }
        bridge.logInfo("[MainWindow] Created all workspaces");
        if (activeIndex >= 0 && activeIndex < tabWidget->count()) {
            tabWidget->setCurrentIndex(activeIndex);
            bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Set active workspace to index " + QString::number(activeIndex));
        } else {
            bridge.logWarning("[MainWindow] MainWindow::loadWorkspaces active workspace index: " + QString::number(activeIndex));
        }
        
        // Update workspace menu
        updateWorkspaceMenu();
        addPlusTab();
        bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Updated workspace menu");
        
        bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Successfully loaded and initialized ");
    } else {
        bridge.logWarning("[MainWindow] MainWindow::loadWorkspaces Failed to load workspaces from configuration");
        
        // Create default workspace if no workspaces were loaded
        if (tabWidget->count() == 0) {
            bridge.logInfo("[MainWindow] MainWindow::loadWorkspaces Creating default workspace");
            createWorkspace();
        }
    }
    
    // Add the "+" tab after loading all workspaces
    
    bridge.logInfo("[MainWindow] Leave MainWindow::loadWorkspaces");
}

void MainWindow::saveWorkspaces() {
    bridge.logInfo("[MainWindow] === Starting workspace saving ===");
    bridge.logInfo("[MainWindow] Current workspace count: " + QString::number(tabWidget->count()));
    
    // Save workspaces through bridge
    if (bridge.saveWorkspaces()) {
        bridge.logInfo("[MainWindow] Successfully saved workspaces to configuration");        
    } else {
        bridge.logError("[MainWindow] Failed to save workspaces to configuration");
    }
    
    bridge.logInfo("[MainWindow] === Workspace saving completed ===");
}

void MainWindow::updateWorkspaceSortIndex() {
    bridge.logInfo("[MainWindow] Enter MainWindow::updateWorkspaceSortIndex");
    
    // Collect workspace IDs in the current tab order
    QVector<int64_t> orderedIds;
    for (int i = 0; i < tabWidget->count(); ++i) {
        if(i == plusTabIndex) {
            continue; // Skip the "+" tab
        }
        Workspace* workspace = qobject_cast<Workspace*>(tabWidget->widget(i));
        if (workspace) {
            int oldIndex = workspace->getSortIndex();
            if(oldIndex != i) {
                workspace->setSortIndex(i);
                bridge.logInfo("[MainWindow] MainWindow::updateWorkspaceSortIndex workspace " + QString::number(workspace->getWorkspaceId()) + " moved from " + QString::number(oldIndex) + " to " + QString::number(i));
            }
        }
    }
    bridge.logInfo("[MainWindow] Leave MainWindow::updateWorkspaceSortIndex");
}

void MainWindow::renameWorkspace()
{
    QMenu *menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) {
        bridge.logError("[MainWindow] Failed to rename workspace: Invalid menu context");
        return;
    }
    
    QAction *action = menu->property("actionSender").value<QAction*>();
    if (!action) {
        bridge.logError("[MainWindow] Failed to rename workspace: Invalid action");
        return;
    }
    
    int index = action->data().toInt();
    if (index < 0 || index >= tabWidget->count()) {
        bridge.logError("[MainWindow] Failed to rename workspace: Invalid index " + QString::number(index));
        return;
    }
    
    Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(index));
    if (!workspace) {
        bridge.logError("[MainWindow] Failed to rename workspace: Invalid workspace at index " + QString::number(index));
        return;
    }
    
    QString oldName = workspace->getDisplayName();
    bridge.logInfo("[MainWindow] Opening rename dialog for workspace '" + oldName + "' at index " + QString::number(index));
    
    bool ok;
    QString newName = QInputDialog::getText(this, 
                                          tr("Rename Workspace"),
                                          tr("Enter new name:"), 
                                          QLineEdit::Normal,
                                          oldName,
                                          &ok);
    
    if (!ok) {
        bridge.logInfo("[MainWindow] Workspace rename cancelled by user");
        return;
    }
    
    if (newName == oldName) {
        bridge.logInfo("[MainWindow] Workspace name unchanged");
        return;
    }
    
    bridge.logInfo("[MainWindow] Attempting to rename workspace from '" + oldName + "' to '" + newName + "'");
    
    // Try to set the new name
    if (workspace->setWorkspaceName(newName)) {
        // Update UI elements
        tabWidget->setTabText(index, newName);
        action->setText(newName);
        updateWorkspaceMenu();
        
        bridge.logInfo("[MainWindow] Successfully renamed workspace from '" + oldName + "' to '" + newName + "'");
    } else {
        bridge.logError("[MainWindow] Failed to rename workspace '" + oldName + "' to '" + newName + "'");
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

void MainWindow::addPlusTab() {
    // Add the "+" tab at the end of the tab widget with a blank widget
    QWidget* plusTabWidget = new QWidget(tabWidget);
    plusTabIndex = tabWidget->addTab(plusTabWidget, "+");
    // Make this tab not closable by removing close buttons on both sides
    tabWidget->tabBar()->setTabButton(plusTabIndex, QTabBar::RightSide, nullptr);
    tabWidget->tabBar()->setTabButton(plusTabIndex, QTabBar::LeftSide, nullptr);
    bridge.logInfo("[MainWindow] MainWindow::addPlusTab '+' tab at index " + QString::number(plusTabIndex));
}

void MainWindow::onTabChanged(int index) {
    // Check if the user clicked on the "+" tab
    if (index == plusTabIndex) {
        // Switch back to the previous tab (if there is one)
        int previousIndex = plusTabIndex > 0 ? plusTabIndex - 1 : 0;
        if (tabWidget->count() > 1) {  // If there's at least one regular tab
            tabWidget->setCurrentIndex(previousIndex);
        }
        bridge.logInfo("[MainWindow] MainWindow::onTabChanged, created new workspace");
    } else if (index >= 0 && index < tabWidget->count()) {
        // Update the active workspace in the bridge
        Workspace *workspace = qobject_cast<Workspace*>(tabWidget->widget(index));
        if (workspace) {
            int64_t workspaceId = workspace->getWorkspaceId();
            bridge.setActiveWorkspace(workspaceId);
            bridge.logInfo("[MainWindow] MainWindow Changed active workspace to " + QString::number(workspaceId));
        }
    }
}

void MainWindow::onTabClicked(int index) {
    // Check if the user clicked on the "+" tab
    if (index == plusTabIndex) {
        // Switch back to the previous tab (if there is one)
        int previousIndex = plusTabIndex > 0 ? plusTabIndex - 1 : 0;
        if (tabWidget->count() > 1) {  // If there's at least one regular tab
            tabWidget->setCurrentIndex(previousIndex);
        }
        
        // Trigger the new workspace action
        createWorkspace();
        
        bridge.logInfo("[MainWindow] MainWindow::onTabClicked, created new workspace");
    }
}

void MainWindow::showAboutDialog() {
    // 创建关于对话框
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(tr("About TxtLogParser"));
    
    // 设置对话框图标
    QPixmap iconPixmap(":/icons/64x64/app_icon.png");
    if (!iconPixmap.isNull()) {
        aboutBox.setIconPixmap(iconPixmap);
    }
    
    // 设置对话框文本内容
    QString aboutText = tr(
        "<h3>TxtLogParser</h3>"
        "<p>Version 1.0.0</p>"
        "<p>A powerful tool for parsing and analyzing text log files.</p>"
        "<p>Copyright © 2025 PanelTree</p>"
        "<p><a href='https://github.com/paneltree/TxtLogParser'>https://github.com/paneltree/TxtLogParser</a></p>"
    );
    
    aboutBox.setText(aboutText);
    
    // 设置详细信息
    QString detailedText = tr(
        "This software is provided under the terms of the MIT License.\n\n"
        "Built with Qt 6 and C++.\n"
        "Thank you for using TxtLogParser!"
    );
    
    aboutBox.setDetailedText(detailedText);
    
    // 设置对话框按钮
    aboutBox.setStandardButtons(QMessageBox::Ok);
    
    // 设置对话框样式，保持与应用程序一致
    aboutBox.setStyleSheet(R"(
        QMessageBox {
            background-color: palette(window);
            color: palette(text);
        }
        QLabel {
            color: palette(text);
        }
        QPushButton {
            background-color: palette(button);
            color: palette(text);
            border: 1px solid palette(mid);
            padding: 5px 15px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: palette(mid);
        }
        QPushButton:pressed {
            background-color: palette(dark);
        }
    )");
    
    // 显示对话框
    aboutBox.exec();
    
    bridge.logInfo("[MainWindow] MainWindow Showed About dialog");
}
