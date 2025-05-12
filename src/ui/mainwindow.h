#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QAction>
#include <QMenu>
#include <QTranslator>
#include <QSettings>
#include <QInputDialog>
#include <QFile>
#include <QList>
#include "../bridge/QtBridge.h"

class QTabWidget;
class QActionGroup;
class QAction;
class QMenu;
class Workspace;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // Make saveWorkspaces public so it can be called from Workspace
    void saveWorkspaces();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void createWorkspace();
    void closeWorkspace();
    void switchToChinese();
    void switchToEnglish();
    void retranslateUi();
    void updateWorkspaceMenu();
    void showWorkspaceContextMenu(QAction* actionSender);
    void renameWorkspace();
    void closeWorkspaceByAction();
    void closeWorkspaceAtIndex(int index);
    void showAboutDialog(); // 添加显示About对话框的槽函数
    void updateStyles(); // 处理系统主题变化的槽函数

private:
    void createMenus();
    void createLanguageMenu();
    void saveSettings();
    void loadSettings();
    void restoreWorkspaces(const QStringList &workspaceNames);
    void loadWorkspaces();
    void updateWorkspaceSortIndex();
    void addPlusTab();
    void onTabChanged(int index);
    void onTabClicked(int index);
    
    QTabWidget *tabWidget;
    QMenu *workspaceMenu;
    QMenu *languageMenu;
    QMenu *helpMenu; // 添加Help菜单
    QAction *newWorkspaceAction;
    QAction *closeWorkspaceAction;
    QAction *aboutAction; // 添加About菜单项
    QTranslator translator;
    QSettings settings;
    
    QActionGroup *workspaceActionGroup;
    QList<QAction*> workspaceActions;
    int plusTabIndex; // Index of the "+" tab

    // Window geometry methods
    void saveWindowGeometry();
    void restoreWindowGeometry();
    
    // Bridge to core functionality
    QtBridge& bridge;
};

#endif // MAINWINDOW_H
