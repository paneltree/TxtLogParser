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

private:
    void createMenus();
    void createToolBars();
    void createLanguageMenu();
    void saveSettings();
    void loadSettings();
    void restoreWorkspaces(const QStringList &workspaceNames);
    void loadWorkspaces();
    void updateWorkspaceSortIndex();
    
    QTabWidget *tabWidget;
    QMenu *workspaceMenu;
    QMenu *languageMenu;
    QAction *newWorkspaceAction;
    QAction *closeWorkspaceAction;
    QTranslator translator;
    QSettings settings;
    void updateWorkspaceNames();
    
    QActionGroup *workspaceActionGroup;
    QList<QAction*> workspaceActions;

    // Window geometry methods
    void saveWindowGeometry();
    void restoreWindowGeometry();
    
    // Bridge to core functionality
    QtBridge& bridge;
};

#endif // MAINWINDOW_H
