#ifndef SEARCHLISTWIDGET_H
#define SEARCHLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QFrame>
#include <QColor>
#include "../bridge/QtBridge.h"
#include "../models/searchconfig.h"

class SearchDialog;
class SearchItemWidget;

class SearchListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SearchListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent = nullptr);
    void initializeWithData(int64_t workspaceId);
    void addSearch(SearchConfig &search);
    QList<SearchConfig> getSearchConfigs() const;
    void doUpdate();

signals:
    void searchsChanged();
    void navigateToNextMatch(int searchId);
    void navigateToPreviousMatch(int searchId);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void showAddSearchDialog();
    void showEditSearchDialog();
    void removeSelectedSearch();
    void updateSearch(int index, const SearchConfig &search);
    void onNavigateToNextMatch(int searchId);
    void onNavigateToPreviousMatch(int searchId);
    void onSearchRemoved(const QString &searchPattern);
    void handleItemMoved(int fromRow, int toRow);

private:
    int64_t workspaceId = -1;
    QtBridge& bridge;
    QListWidget *searchListWidget;
    QList<SearchConfig> searchList;
    int colorIndex; // To track which color to use next
    
    static const QList<QColor> predefinedColors; // List of predefined colors
    
    SearchDialog *createSearchDialog(const QString &title, const SearchConfig &initialSearch = SearchConfig());
    void createSearchItem(int index, const SearchConfig &search);
    void updateSearchRows();
    
    // Helper methods for search processing
    int countMatches(const QString &content, const SearchConfig &search);
    void applySearchs(const QString &content);
};

// Custom widget for search items
class SearchItemWidget : public QWidget {
    Q_OBJECT
public:
    SearchItemWidget(const SearchConfig &search, int index, QWidget *parent = nullptr);
    SearchConfig getSearchConfig() const { return currentSearch; }
    void updateMatchCount(int count);
    void setSearchIndex(int index);
    void setSearchConfig(const SearchConfig &search);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

signals:
    void searchChanged(int index, const SearchConfig &search);
    void editRequested(int index);
    void removeRequested(int index);
    void navigateToNextMatch(int searchId);
    void navigateToPreviousMatch(int searchId);

private slots:
    void onCaseSensitiveToggled(bool checked);
    void onWholeWordToggled(bool checked);
    void onRegexToggled(bool checked);
    void onColorClicked();
    void onEnabledToggled(bool checked);
    void onNextMatchClicked();
    void onPrevMatchClicked();
    void showEditDialog();

private:
    QLabel *searchLabel;
    QCheckBox *enabledButton;
    QCheckBox *caseSensitiveButton;
    QCheckBox *wholeWordButton;
    QCheckBox *regexButton;
    QPushButton *colorButton;
    QPushButton *removeButton;
    
    // New UI elements for match count and navigation
    QLabel *matchCountLabel;
    QPushButton *prevMatchButton;
    QPushButton *nextMatchButton;
    
    SearchConfig currentSearch;
    int itemIndex;
    
    void updateEnabledState();
    void updateSearchStyle();
};

// Dialog for adding/editing searchs
class SearchDialog : public QDialog {
    Q_OBJECT
public:
    SearchDialog(const QString &title, QWidget *parent = nullptr);
    
    void setSearchConfig(const SearchConfig &search);
    SearchConfig getSearchConfig() const;
    
private slots:
    void selectColor();
    
private:
    qint32 m_searchId = -1;
    qint32 m_searchRow = -1;
    QLineEdit *searchStringEdit;
    QCheckBox *enabledCheckBox;
    QCheckBox *caseSensitiveCheckBox;
    QCheckBox *wholeWordCheckBox;
    QCheckBox *regexCheckBox;
    QPushButton *colorButton;
    QColor currentColor;
    
    void updateColorButton();
};

#endif // SEARCHLISTWIDGET_H 