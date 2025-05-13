#include "filterlistwidget.h"
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <utility>  // For std::as_const
#include "../../bridge/QtBridge.h"
#include "../../bridge/FilterAdapter.h"
#include "../../core/FilterData.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "../../utils/GenericGuard.h"
#include <QMimeData>
#include <QApplication>  // Add QApplication include
#include "../StyleManager.h" // Include StyleManager


///////////////////////////////////////////////////////////////////////////
//                      FilterListWidget implementation
///////////////////////////////////////////////////////////////////////////


FilterListWidget::FilterListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent)
    : QWidget(parent), workspaceId(workspaceId), colorIndex(0), bridge(bridge)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel(tr("Filters"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #0078d4;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    filterListWidget = new QListWidget(this);
    // Set item height to accommodate the custom widget
    filterListWidget->setStyleSheet("QListWidget::item { min-height: 40px; }");
    
    // Enable drag and drop functionality
    filterListWidget->setDragEnabled(true);
    filterListWidget->setAcceptDrops(true);
    filterListWidget->setDropIndicatorShown(true);
    filterListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    
    // Connect signal for item movement
    connect(filterListWidget->model(), &QAbstractItemModel::rowsMoved,
            this, [this](const QModelIndex&, int start, int end, const QModelIndex&, int destination) {
                // Since we only move one item at a time, start and end will be the same
                this->handleItemMoved(start, destination);
            });
    
    layout->addWidget(filterListWidget);
    
    setLayout(layout);
    
    // Connect to StyleManager for theme changes
    connect(&StyleManager::instance(), &StyleManager::stylesChanged, 
            this, &FilterListWidget::updateWidgetStyles);
}

void FilterListWidget::initializeWithData(int64_t workspaceId) {
    // Clear existing filters
    filterList.clear();
    filterListWidget->clear();
    bridge.getFilterListFrmWorkspace(workspaceId, [this](const QList<FilterConfig> &filters) {
        for (const FilterConfig &filter : filters) {
            assert(static_cast<qint32>(filterList.size()) == filter.filterRow);
            filterList.append(filter);
            createFilterItem(filterList.size() - 1, filter);
        }
    });
} 

void FilterListWidget::addFilter(FilterConfig &filter)
{
    // Check if the filter string is empty
    if (filter.filterPattern.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Filter"), tr("Filter string cannot be empty."));
        return;
    }

    int32_t filterRow = filterList.size();
    filter.filterRow = filterRow;
    int32_t filterId = bridge.addFilterToWorkspace(workspaceId, filter);
    if(filterId >= 0) {
        filter.filterId = filterId;
        filterList.append(filter);
        createFilterItem(filterList.size() - 1, filter);
        emit filtersChanged();
    } else {
        QtBridge::getInstance().logError(QString("Failed to add filter: %1").arg(filter.filterPattern));
    }
}

void FilterListWidget::createFilterItem(int index, const FilterConfig &filter)
{
    QListWidgetItem *item = new QListWidgetItem(filterListWidget);
    // Set size to accommodate the custom widget
    item->setSizeHint(QSize(0, 40));
    filterListWidget->addItem(item);
    
    // Create custom widget
    FilterItemWidget *filterItemWidget = new FilterItemWidget(filter, index, filterListWidget);
    
    // Connect signals
    connect(filterItemWidget, &FilterItemWidget::filterChanged, this, &FilterListWidget::updateFilter);
    connect(filterItemWidget, &FilterItemWidget::editRequested, this, [this](int idx) {
        filterListWidget->setCurrentRow(idx);
        showEditFilterDialog();
    });
    connect(filterItemWidget, &FilterItemWidget::removeRequested, this, [this](int idx) {
        filterListWidget->setCurrentRow(idx);
        removeSelectedFilter();
    });
    connect(filterItemWidget, &FilterItemWidget::navigateToNextMatch, this, &FilterListWidget::onNavigateToNextMatch);
    connect(filterItemWidget, &FilterItemWidget::navigateToPreviousMatch, this, &FilterListWidget::onNavigateToPreviousMatch);
    
    // Set the custom widget for the item
    filterListWidget->setItemWidget(item, filterItemWidget);
}

void FilterListWidget::updateFilter(int index, const FilterConfig &filter)
{
    // Update the filter in our list
    if (index >= 0 && index < filterList.size()) {
        filterList[index] = filter;
    }
    bridge.updateFilterInWorkspace(workspaceId, filter);
    
    // Emit signal that filters have changed
    emit filtersChanged();
}

void FilterListWidget::doUpdate()
{
    QMap<int, int> matchCounts = bridge.getFilterMatchCounts(workspaceId);
    // Update match counts for each filter
    for (int i = 0; i < filterListWidget->count(); i++) {
        QListWidgetItem *item = filterListWidget->item(i);
        FilterItemWidget *widget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
        if (widget) {
            if(matchCounts.contains(filterList[i].filterId)){
                widget->updateMatchCount(matchCounts[filterList[i].filterId]);
            }else{
                widget->updateMatchCount(0);
            }
        }
    }
}

void FilterListWidget::onNavigateToNextMatch(int filterId)
{
    emit navigateToNextMatch(filterId);
}

void FilterListWidget::onNavigateToPreviousMatch(int filterId)
{
    emit navigateToPreviousMatch(filterId);
}

QList<FilterConfig> FilterListWidget::getFilterConfigs() const
{
    return filterList;
}

void FilterListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    
    // 设置菜单样式，包括鼠标悬停效果
    menu.setStyleSheet(R"(
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
    
    QAction *addAction = menu.addAction(tr("Add Filter"));
    
    QListWidgetItem *currentItem = filterListWidget->currentItem();
    QAction *editAction = menu.addAction(tr("Edit Filter"));
    QAction *removeAction = menu.addAction(tr("Remove Filter"));
    
    // Only enable edit/remove if an item is selected
    bool hasSelection = (currentItem != nullptr);
    editAction->setEnabled(hasSelection);
    removeAction->setEnabled(hasSelection);
    
    QAction *selectedAction = menu.exec(event->globalPos());
    if (selectedAction == addAction) {
        showAddFilterDialog();
    } else if (selectedAction == editAction && hasSelection) {
        showEditFilterDialog();
    } else if (selectedAction == removeAction && hasSelection) {
        removeSelectedFilter();
    }
}

void FilterListWidget::showAddFilterDialog()
{
    FilterConfig filter;
    filter.color = bridge.getNextFilterColor(workspaceId);
    FilterDialog *dialog = createFilterDialog(tr("Add Filter"), filter);
    
    if (dialog->exec() == QDialog::Accepted) {
        FilterConfig filter = dialog->getFilterConfig();
        addFilter(filter);
    }
    
    delete dialog;
}

void FilterListWidget::showEditFilterDialog()
{
    int currentRow = filterListWidget->currentRow();
    if (currentRow < 0 || currentRow >= filterList.size()) {
        return;
    }
    
    FilterConfig currentFilter = filterList.at(currentRow);
    FilterDialog *dialog = createFilterDialog(tr("Edit Filter"), currentFilter);
    
    if (dialog->exec() == QDialog::Accepted) {
        FilterConfig updatedFilter = dialog->getFilterConfig();
        
        // Check if the filter string is empty
        if (updatedFilter.filterPattern.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Filter"), tr("Filter string cannot be empty."));
            delete dialog;
            return;
        }
        
        // Update the filter in the list
        filterList[currentRow] = updatedFilter;
        
        // Update the widget
        QListWidgetItem *item = filterListWidget->item(currentRow);
        if (item) {
            // Remove the old widget
            QWidget *oldWidget = filterListWidget->itemWidget(item);
            if (oldWidget) {
                filterListWidget->removeItemWidget(item);
                delete oldWidget;
            }
            
            // Create a new widget with updated filter
            FilterItemWidget *filterItemWidget = new FilterItemWidget(updatedFilter, currentRow, filterListWidget);
            
            GenericGuard guard(
                [&]() {
                    bridge.beginWorkspaceUpdate();
                    bridge.beginFilterUpdate(workspaceId);
                },
                [&]() {
                    bridge.commitWorkspaceUpdate();
                    bridge.commitFilterUpdate(workspaceId);
                },
                [&]() {
                    bridge.rollbackWorkspaceUpdate();
                    bridge.rollbackFilterUpdate(workspaceId);
                }
            );
            bridge.updateFilterInWorkspace(workspaceId, updatedFilter);
            guard.commit();
            // Connect signals
            connect(filterItemWidget, &FilterItemWidget::filterChanged, this, &FilterListWidget::updateFilter);
            connect(filterItemWidget, &FilterItemWidget::editRequested, this, [this](int idx) {
                filterListWidget->setCurrentRow(idx);
                showEditFilterDialog();
            });
            connect(filterItemWidget, &FilterItemWidget::removeRequested, this, [this](int idx) {
                filterListWidget->setCurrentRow(idx);
                removeSelectedFilter();
            });
            connect(filterItemWidget, &FilterItemWidget::navigateToNextMatch, this, &FilterListWidget::onNavigateToNextMatch);
            connect(filterItemWidget, &FilterItemWidget::navigateToPreviousMatch, this, &FilterListWidget::onNavigateToPreviousMatch);
            
            // Set the custom widget for the item
            filterListWidget->setItemWidget(item, filterItemWidget);
            
            QtBridge::getInstance().logInfo(QString("Updated filter: %1").arg(updatedFilter.filterPattern));
            
            // Emit signal that filters have changed
            emit filtersChanged();
        }
    }
    
    delete dialog;
}

void FilterListWidget::removeSelectedFilter()
{
    int currentRow = filterListWidget->currentRow();
    assert(currentRow >= 0 && currentRow < filterList.size());
    GenericGuard guard(
        [&]() {
            bridge.beginWorkspaceUpdate();
            bridge.beginFilterUpdate(workspaceId);
        },
        [&]() {
            bridge.commitWorkspaceUpdate();
            bridge.commitFilterUpdate(workspaceId);
        },
        [&]() {
            bridge.rollbackWorkspaceUpdate();
            bridge.rollbackFilterUpdate(workspaceId);
        }
    );

    bridge.removeFilterFromWorkspace(workspaceId, filterList[currentRow].filterId);
    filterList.removeAt(currentRow);
    QListWidgetItem *item = filterListWidget->takeItem(currentRow);
    delete item;
    //update the indices of the remaining filter widgets

    QList<qint32> filterIds;
    for (int i = 0; i < filterListWidget->count(); i++) {
        QListWidgetItem *item = filterListWidget->item(i);
        FilterItemWidget *widget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
        if (widget) {
            widget->setFilterIndex(i);
            filterIds.append(filterList[i].filterId);
        }
    }
    bridge.updateFilterRowsInWorkspace(workspaceId, filterIds);
    guard.commit();
    emit filtersChanged();
}

FilterDialog* FilterListWidget::createFilterDialog(const QString &title, const FilterConfig &initialFilter)
{
    FilterDialog *dialog = new FilterDialog(title, this);
    dialog->setFilterConfig(initialFilter);
    
    return dialog;
}

void FilterListWidget::onFilterRemoved(const QString &filterPattern)
{
    // Find the filter in the list
    int indexToRemove = -1;
    for (int i = 0; i < filterList.size(); ++i) {
        if (filterList[i].filterPattern == filterPattern) {
            indexToRemove = i;
            break;
        }
    }

    if (indexToRemove >= 0) {
        // Remove the filter from the list
        filterList.removeAt(indexToRemove);

        // Remove the item from the list widget
        QListWidgetItem *item = filterListWidget->takeItem(indexToRemove);
        delete item;

        QtBridge::getInstance().logInfo(QString("Removed filter: %1").arg(filterPattern));
        QtBridge::getInstance().troubleshootingLog("FILTER", "REMOVE", filterPattern);

        // Update the indices of the remaining filter widgets
        for (int i = 0; i < filterListWidget->count(); i++) {
            QListWidgetItem *item = filterListWidget->item(i);
            FilterItemWidget *widget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
            if (widget) {
                widget->setFilterIndex(i);
            }
        }

        // Emit signal that filters have changed
        emit filtersChanged();
    }
}

// Helper method to count matches in content
int FilterListWidget::countMatches(const QString &content, const FilterConfig &filter)
{
    int count = 0;

    if (filter.isRegex) {
        // Use regular expression
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!filter.caseSensitive) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }

        QString pattern = filter.filterPattern;
        if (filter.wholeWord) {
            pattern = "\\b" + pattern + "\\b";
        }

        QRegularExpression regex(pattern, options);
        QRegularExpressionMatchIterator matches = regex.globalMatch(content);

        while (matches.hasNext()) {
            matches.next();
            count++;
        }
    } else {
        // Use string search
        Qt::CaseSensitivity cs = filter.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

        if (filter.wholeWord) {
            // Split content into words and count matches
            QStringList words = content.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
            for (const QString &word : words) {
                if (word.compare(filter.filterPattern, cs) == 0) {
                    count++;
                }
            }
        } else {
            // Count all occurrences
            int pos = 0;
            while ((pos = content.indexOf(filter.filterPattern, pos, cs)) != -1) {
                count++;
                pos += filter.filterPattern.length();
            }
        }
    }

    return count;
}

// Process filters against content
void FilterListWidget::applyFilters(const QString &content)
{
    // Process each filter
    for (int i = 0; i < filterList.size(); ++i) {
        FilterConfig &filter = filterList[i];

        // Skip disabled filters
        if (!filter.enabled) {
            continue;
        }

        // Count matches
        int matchCount = countMatches(content, filter);
        filter.matchCount = matchCount;

        // Update the UI item
        QListWidgetItem *item = filterListWidget->item(i);
        if (item) {
            FilterItemWidget *filterItemWidget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
            if (filterItemWidget) {
                filterItemWidget->updateMatchCount(matchCount);
            }
        }
    }

    // Emit signal that filters have been processed
    emit filtersChanged();
}

void FilterListWidget::handleItemMoved(int fromRow, int toRow)
{
    updateFilterRows();
    emit filtersChanged();
}

void FilterListWidget::updateFilterRows()
{
    GenericGuard guard(
        [&]() {
            bridge.beginWorkspaceUpdate();
            bridge.beginFilterUpdate(workspaceId);
        },
        [&]() {
            bridge.commitWorkspaceUpdate();
            bridge.commitFilterUpdate(workspaceId);
        },
        [&]() {
            bridge.rollbackWorkspaceUpdate();
            bridge.rollbackFilterUpdate(workspaceId);
        }
    );
    filterList.clear();
    QList<qint32> filterIds;
    for (int i = 0; i < filterListWidget->count(); i++) {
        QListWidgetItem *item = filterListWidget->item(i);
        if (item) {
            FilterItemWidget *widget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
            if (widget) {
                widget->setFilterIndex(i);
                FilterConfig filter = widget->getFilterConfig();
                filterList.append(filter);
                filterIds.append(filter.filterId);
            }
        }
    }
    QtBridge::getInstance().updateFilterRowsInWorkspace(workspaceId, filterIds);
    guard.commit();
}

void FilterListWidget::updateWidgetStyles()
{
    // Update styles for all filter item widgets
    for (int i = 0; i < filterListWidget->count(); i++) {
        QListWidgetItem *item = filterListWidget->item(i);
        FilterItemWidget *widget = qobject_cast<FilterItemWidget*>(filterListWidget->itemWidget(item));
        if (widget) {
            widget->applySystemStyles();
        }
    }
}

///////////////////////////////////////////////////////////////////////////
//                      FilterDialog implementation
///////////////////////////////////////////////////////////////////////////

FilterDialog::FilterDialog(const QString &title, QWidget *parent)
    : QDialog(parent), currentColor(QColor("#FF4444"))  // Default to red
{
    setWindowTitle(title);
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout for the filter properties
    QFormLayout *formLayout = new QFormLayout();

    // Filter string input
    filterStringEdit = new QLineEdit(this);
    filterStringEdit->setMinimumWidth(400);
    formLayout->addRow(tr("Filter:"), filterStringEdit);

    // Enabled checkbox
    enabledCheckBox = new QCheckBox(tr("Enabled"), this);
    enabledCheckBox->setChecked(true);
    formLayout->addRow(tr("Status:"), enabledCheckBox);

    // Checkboxes for options
    caseSensitiveCheckBox = new QCheckBox(tr("Case Sensitive"), this);
    wholeWordCheckBox = new QCheckBox(tr("Whole Word"), this);
    regexCheckBox = new QCheckBox(tr("Regular Expression"), this);

    QVBoxLayout *optionsLayout = new QVBoxLayout();
    optionsLayout->addWidget(caseSensitiveCheckBox);
    optionsLayout->addWidget(wholeWordCheckBox);
    optionsLayout->addWidget(regexCheckBox);

    formLayout->addRow(tr("Options:"), optionsLayout);

    // Color selection
    colorButton = new QPushButton(tr("Select Color"), this);
    updateColorButton();
    connect(colorButton, &QPushButton::clicked, this, &FilterDialog::selectColor);

    formLayout->addRow(tr("Color:"), colorButton);

    mainLayout->addLayout(formLayout);

    // Add standard buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Validate input before accepting
    connect(this, &QDialog::accepted, this, [this]() {
        if (filterStringEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Filter string cannot be empty."));
            filterStringEdit->setFocus();
            reject();
        }
    });

    setLayout(mainLayout);
}

void FilterDialog::updateColorButton()
{
    // Set both background color and text color for better visibility
    QColor bgColor(currentColor);

    // Calculate contrasting text color (white for dark backgrounds, black for light)
    int brightness = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;
    QColor textColor = (brightness > 128) ? QColor(0, 0, 0) : QColor(255, 255, 255);

    // Set button text to show the color name/value
    colorButton->setText(currentColor.name());

    // Apply stylesheet with both background and text color
    colorButton->setStyleSheet(QString("background-color: %1; color: %2; padding: 5px;")
                              .arg(currentColor.name())
                              .arg(textColor.name()));
}

void FilterDialog::setFilterConfig(const FilterConfig &filter)
{
    m_filterId = filter.filterId;
    m_filterRow = filter.filterRow;
    filterStringEdit->setText(filter.filterPattern);
    enabledCheckBox->setChecked(filter.enabled);
    caseSensitiveCheckBox->setChecked(filter.caseSensitive);
    wholeWordCheckBox->setChecked(filter.wholeWord);
    regexCheckBox->setChecked(filter.isRegex);
    currentColor = filter.color;
    updateColorButton();
}

FilterConfig FilterDialog::getFilterConfig() const
{
    return FilterConfig(
        m_filterId,
        m_filterRow,
        filterStringEdit->text(),
        caseSensitiveCheckBox->isChecked(),
        wholeWordCheckBox->isChecked(),
        regexCheckBox->isChecked(),
        enabledCheckBox->isChecked(),
        currentColor
    );
}

void FilterDialog::selectColor()
{
    QColor color = QColorDialog::getColor(currentColor, this, tr("Select Filter Color"));

    if (color.isValid()) {
        currentColor = color;
        updateColorButton();
    }
}
///////////////////////////////////////////////////////////////////////////
//                      FilterItemWidget implementation
///////////////////////////////////////////////////////////////////////////

FilterItemWidget::FilterItemWidget(const FilterConfig &filter, int index, QWidget *parent)
    : QWidget(parent), currentFilter(filter), itemIndex(index)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    
    // Enable checkbox
    enabledButton = new QCheckBox("", this);
    enabledButton->setChecked(filter.enabled);
    enabledButton->setToolTip(tr("Enable/Disable Filter"));
    connect(enabledButton, &QCheckBox::toggled, this, &FilterItemWidget::onEnabledToggled);
    layout->addWidget(enabledButton);
    
    // Filter text label
    filterLabel = new QLabel(filter.filterPattern, this);
    filterLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(filter.color.name()));
    filterLabel->setCursor(Qt::PointingHandCursor);
    filterLabel->setToolTip(filter.filterPattern);
    filterLabel->setWordWrap(false);
    filterLabel->setTextFormat(Qt::PlainText);
    
    layout->addWidget(filterLabel, 3); // Increase stretch factor from 1 to 3
    
    // Match count label
    matchCountLabel = new QLabel(tr("M: 0"), this);
    matchCountLabel->setAlignment(Qt::AlignCenter);
    matchCountLabel->setMinimumWidth(40); // Reduce from 80 to 40
    matchCountLabel->setMaximumWidth(60); // Add maximum width
    layout->addWidget(matchCountLabel);
    
    // Navigation buttons
    prevMatchButton = new QPushButton("◀", this);
    prevMatchButton->setFixedSize(20, 24); // Reduce width from 24 to 20
    prevMatchButton->setToolTip(tr("Previous Match"));
    prevMatchButton->setEnabled(false);
    connect(prevMatchButton, &QPushButton::clicked, this, &FilterItemWidget::onPrevMatchClicked);
    layout->addWidget(prevMatchButton);
    
    nextMatchButton = new QPushButton("▶", this);
    nextMatchButton->setFixedSize(20, 24); // Reduce width from 24 to 20
    nextMatchButton->setToolTip(tr("Next Match"));
    nextMatchButton->setEnabled(false);
    connect(nextMatchButton, &QPushButton::clicked, this, &FilterItemWidget::onNextMatchClicked);
    layout->addWidget(nextMatchButton);
    
    // Case sensitive button - replace checkbox with toolbutton
    caseSensitiveButton = new QToolButton(this);
    caseSensitiveButton->setText("Aa");
    caseSensitiveButton->setCheckable(true);
    caseSensitiveButton->setChecked(filter.caseSensitive);
    caseSensitiveButton->setToolTip(tr("Case Sensitive"));
    caseSensitiveButton->setFixedSize(24, 24);
    //caseSensitiveButton->setMaximumWidth(30);
    connect(caseSensitiveButton, &QToolButton::toggled, this, &FilterItemWidget::onCaseSensitiveToggled);
    layout->addWidget(caseSensitiveButton);
    
    // Whole word button - replace checkbox with toolbutton
    wholeWordButton = new QToolButton(this);
    wholeWordButton->setText("ab");
    wholeWordButton->setCheckable(true);
    wholeWordButton->setChecked(filter.wholeWord);
    wholeWordButton->setToolTip(tr("Whole Word"));
    wholeWordButton->setFixedSize(24, 24);
    //wholeWordButton->setMaximumWidth(30);
    connect(wholeWordButton, &QToolButton::toggled, this, &FilterItemWidget::onWholeWordToggled);
    layout->addWidget(wholeWordButton);
    
    // Regex button - replace checkbox with toolbutton
    regexButton = new QToolButton(this);
    regexButton->setText(".*");
    regexButton->setCheckable(true);
    regexButton->setChecked(filter.isRegex);
    regexButton->setToolTip(tr("Regular Expression"));
    regexButton->setFixedSize(24, 24);
    regexButton->setMaximumWidth(30);
    connect(regexButton, &QToolButton::toggled, this, &FilterItemWidget::onRegexToggled);
    layout->addWidget(regexButton);
    
    // Color button
    colorButton = new QPushButton(this);
    colorButton->setFixedSize(20, 24); // Reduce width from 24 to 20
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(filter.color.name()));
    connect(colorButton, &QPushButton::clicked, this, &FilterItemWidget::onColorClicked);
    layout->addWidget(colorButton);
    
    // Remove button
    removeButton = new QPushButton(QIcon::fromTheme("edit-delete"), "", this);
    removeButton->setFixedSize(20, 24); // Reduce width from 24 to 20
    removeButton->setToolTip(tr("Remove Filter"));
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeRequested(itemIndex);
    });
    layout->addWidget(removeButton);
    
    setLayout(layout);
    
    // Connect double-click on the filter label to edit
    connect(filterLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit editRequested(itemIndex);
    });
    
    // Install event filter to handle mouse events
    filterLabel->installEventFilter(this);
    
    // Apply system theme styles
    applySystemStyles();
}

// Add event filter to handle double-click on filter label
bool FilterItemWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == filterLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            emit editRequested(itemIndex);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FilterItemWidget::updateMatchCount(int count)
{
    currentFilter.matchCount = count;
    matchCountLabel->setText(tr("M: %1").arg(count));
    
    // Enable/disable navigation buttons based on match count
    bool hasMatches = (count > 0);
    prevMatchButton->setEnabled(hasMatches);
    nextMatchButton->setEnabled(hasMatches);
    
    // Update tooltip to show current position if there are matches
    if (hasMatches) {
        int position = currentFilter.currentMatchPosition + 1;
        // Ensure position is within valid range
        if (position < 1) position = 1;
        if (position > count) position = count;
        
        matchCountLabel->setToolTip(tr("Match %1 of %2").arg(position).arg(count));
    } else {
        matchCountLabel->setToolTip(tr("No matches found"));
        // Reset current position when no matches
        currentFilter.currentMatchPosition = 0;
    }
}

void FilterItemWidget::setFilterIndex(int index) {
    itemIndex = index;
    currentFilter.filterRow = index;
}

void FilterItemWidget::onNextMatchClicked()
{
    emit navigateToNextMatch(currentFilter.filterId);
}

void FilterItemWidget::onPrevMatchClicked()
{
    emit navigateToPreviousMatch(currentFilter.filterId);
}

void FilterItemWidget::onEnabledToggled(bool checked)
{
    currentFilter.enabled = checked;
    updateEnabledState();
    emit filterChanged(itemIndex, currentFilter);
}

void FilterItemWidget::updateEnabledState()
{
    bool isEnabled = currentFilter.enabled;
    
    QString styleSheet = QString("color: %1; font-weight: bold;").arg(currentFilter.color.name());
    if (!isEnabled) {
        styleSheet += " opacity: 0.5;";
    }
    filterLabel->setStyleSheet(styleSheet);
    
    caseSensitiveButton->setEnabled(isEnabled);
    if (isEnabled) {
        updateCaseSensitiveButtonStyle();
    } else {
        QPalette palette = QApplication::palette();
        caseSensitiveButton->setStyleSheet(QString("QToolButton { background-color: %1; border: 1px solid %2; border-radius: 3px; opacity: 0.5; }")
            .arg(palette.color(QPalette::Button).name())
            .arg(palette.color(QPalette::Mid).name()));
    }
    
    wholeWordButton->setEnabled(isEnabled);
    if (isEnabled) {
        updateWholeWordButtonStyle();
    } else {
        QPalette palette = QApplication::palette();
        wholeWordButton->setStyleSheet(QString("QToolButton { background-color: %1; border: 1px solid %2; border-radius: 3px; opacity: 0.5; text-decoration: underline; }")
            .arg(palette.color(QPalette::Button).name())
            .arg(palette.color(QPalette::Mid).name()));
    }
    
    regexButton->setEnabled(isEnabled);
    if (isEnabled) {
        updateRegexButtonStyle();
    } else {
        QPalette palette = QApplication::palette();
        regexButton->setStyleSheet(QString("QToolButton { background-color: %1; border: 1px solid %2; border-radius: 3px; opacity: 0.5; }")
            .arg(palette.color(QPalette::Button).name())
            .arg(palette.color(QPalette::Mid).name()));
    }
    
    colorButton->setEnabled(isEnabled);
}

void FilterItemWidget::onCaseSensitiveToggled(bool checked)
{
    currentFilter.caseSensitive = checked;
    updateCaseSensitiveButtonStyle();
    emit filterChanged(itemIndex, currentFilter);
}

void FilterItemWidget::onWholeWordToggled(bool checked)
{
    currentFilter.wholeWord = checked;
    updateWholeWordButtonStyle();
    emit filterChanged(itemIndex, currentFilter);
}

void FilterItemWidget::onRegexToggled(bool checked)
{
    currentFilter.isRegex = checked;
    updateRegexButtonStyle();
    emit filterChanged(itemIndex, currentFilter);
}

void FilterItemWidget::onColorClicked()
{
    QColor color = QColorDialog::getColor(currentFilter.color, this, tr("Select Filter Color"));
    
    if (color.isValid()) {
        currentFilter.color = color;
        colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(color.name()));
        filterLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color.name()));
        emit filterChanged(itemIndex, currentFilter);
    }
}

// Add this method to the FilterItemWidget class
void FilterItemWidget::setFilterConfig(const FilterConfig &filter)
{
    currentFilter = filter;
    itemIndex = filter.filterRow; // Update the item index to match the filter row
    
    filterLabel->setText(filter.filterPattern);
    filterLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(filter.color.name()));
    filterLabel->setToolTip(filter.filterPattern);
    
    // Update button states
    enabledButton->setChecked(filter.enabled);
    caseSensitiveButton->setChecked(filter.caseSensitive);
    updateCaseSensitiveButtonStyle();
    wholeWordButton->setChecked(filter.wholeWord);
    updateWholeWordButtonStyle();
    regexButton->setChecked(filter.isRegex);
    
    // Update color button
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(filter.color.name()));
    
    // Update match count
    updateMatchCount(filter.matchCount);
    
    // Update enabled state
    updateEnabledState();
}

void FilterItemWidget::updateFilterStyle()
{
    QString styleSheet = QString("color: %1; font-weight: bold;").arg(currentFilter.color.name());
    filterLabel->setStyleSheet(styleSheet);
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(currentFilter.color.name()));
}

void FilterItemWidget::showEditDialog()
{
    FilterDialog *dialog = new FilterDialog(tr("Edit Filter"), this);
    dialog->setFilterConfig(currentFilter);
    
    if (dialog->exec() == QDialog::Accepted) {
        // Get updated filter configuration
        FilterConfig updatedFilter = dialog->getFilterConfig();
        
        // Check if the filter string is empty
        if (updatedFilter.filterPattern.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Filter"), tr("Filter string cannot be empty."));
            delete dialog;
            return;
        }
        
        // Update current filter
        currentFilter = updatedFilter;
        
        // Update UI
        setFilterConfig(currentFilter);
        
        // Log the update
        QtBridge::getInstance().logInfo(QString("Updated filter: %1").arg(updatedFilter.filterPattern));
        
        // Emit signal that filter has changed
        emit filterChanged(itemIndex, currentFilter);
    }
    
    delete dialog;
}

void FilterItemWidget::updateCaseSensitiveButtonStyle()
{
    caseSensitiveButton->setStyleSheet(StyleManager::instance().getFilterToolButtonStyle(caseSensitiveButton->isChecked()));
}

void FilterItemWidget::updateWholeWordButtonStyle()
{
    QString styleSheet = StyleManager::instance().getFilterToolButtonStyle(wholeWordButton->isChecked());
    // Add text decoration for the whole word button
    styleSheet += " QToolButton { text-decoration: underline; }";
    wholeWordButton->setStyleSheet(styleSheet);
}

void FilterItemWidget::updateRegexButtonStyle()
{
    regexButton->setStyleSheet(StyleManager::instance().getFilterToolButtonStyle(regexButton->isChecked()));
}

void FilterItemWidget::applySystemStyles()
{
    // Get styles from StyleManager
    const StyleManager& styleManager = StyleManager::instance();
    
    // Apply navigation button styles
    prevMatchButton->setStyleSheet(styleManager.getFilterNavigationButtonStyle());
    nextMatchButton->setStyleSheet(styleManager.getFilterNavigationButtonStyle());
    
    // Apply tool button styles based on their checked state
    updateCaseSensitiveButtonStyle();
    updateWholeWordButtonStyle();
    updateRegexButtonStyle();
    
    // Apply match count label style
    matchCountLabel->setStyleSheet(styleManager.getMatchCountLabelStyle());
    
    // Keep filter label color unchanged, as it's part of the filter identity
    // Update colorButton, but keep its background color as the filter color
    QString buttonStyleBase = styleManager.getFilterNavigationButtonStyle();
    QString colorButtonStyle = QString("background-color: %1; border: 1px solid #888;").arg(currentFilter.color.name());
    colorButton->setStyleSheet(colorButtonStyle);
    
    // Apply navigation button style to removeButton
    removeButton->setStyleSheet(styleManager.getFilterNavigationButtonStyle());
    
    // Update enabled state to refresh all widgets
    updateEnabledState();
}
