#ifndef FILTERLISTWIDGET_H
#define FILTERLISTWIDGET_H

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
#include "../models/filterconfig.h"

class FilterDialog;
class FilterItemWidget;

class FilterListWidget : public QWidget {
    Q_OBJECT
public:
    explicit FilterListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent = nullptr);
    void initializeWithData(int64_t workspaceId);
    void addFilter(FilterConfig &filter);
    QList<FilterConfig> getFilterConfigs() const;
    void doUpdate();

signals:
    void filtersChanged();
    void navigateToNextMatch(int filterId);
    void navigateToPreviousMatch(int filterId);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void showAddFilterDialog();
    void showEditFilterDialog();
    void removeSelectedFilter();
    void updateFilter(int index, const FilterConfig &filter);
    void onNavigateToNextMatch(int filterId);
    void onNavigateToPreviousMatch(int filterId);
    void onFilterRemoved(const QString &filterPattern);
    void handleItemMoved(int fromRow, int toRow);

private:
    int64_t workspaceId = -1;
    QtBridge& bridge;
    QListWidget *filterListWidget;
    QList<FilterConfig> filterList;
    int colorIndex; // To track which color to use next
    
    static const QList<QColor> predefinedColors; // List of predefined colors
    
    FilterDialog *createFilterDialog(const QString &title, const FilterConfig &initialFilter);
    void createFilterItem(int index, const FilterConfig &filter);
    void updateFilterRows();
    
    // Helper methods for filter processing
    int countMatches(const QString &content, const FilterConfig &filter);
    void applyFilters(const QString &content);
};

// Custom widget for filter items
class FilterItemWidget : public QWidget {
    Q_OBJECT
public:
    FilterItemWidget(const FilterConfig &filter, int index, QWidget *parent = nullptr);
    FilterConfig getFilterConfig() const { return currentFilter; }
    void updateMatchCount(int count);
    void setFilterIndex(int index);
    void setFilterConfig(const FilterConfig &filter);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void filterChanged(int index, const FilterConfig &filter);
    void editRequested(int index);
    void removeRequested(int index);
    void navigateToNextMatch(int filterId);
    void navigateToPreviousMatch(int filterId);

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
    QLabel *filterLabel;
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
    
    FilterConfig currentFilter;
    int itemIndex;
    
    void updateEnabledState();
    void updateFilterStyle();
};

// Dialog for adding/editing filters
class FilterDialog : public QDialog {
    Q_OBJECT
public:
    FilterDialog(const QString &title, QWidget *parent = nullptr);
    
    void setFilterConfig(const FilterConfig &filter);
    FilterConfig getFilterConfig() const;
    
private slots:
    void selectColor();
    
private:
    qint32 m_filterId = -1;
    qint32 m_filterRow = -1;
    QLineEdit *filterStringEdit;
    QCheckBox *enabledCheckBox;
    QCheckBox *caseSensitiveCheckBox;
    QCheckBox *wholeWordCheckBox;
    QCheckBox *regexCheckBox;
    QPushButton *colorButton;
    QColor currentColor;
    
    void updateColorButton();
};

#endif // FILTERLISTWIDGET_H 