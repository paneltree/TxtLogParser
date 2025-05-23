#include "searchlistwidget.h"
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <utility>  // For std::as_const
#include "../../bridge/QtBridge.h"
#include "../../bridge/SearchAdapter.h"
#include "../../core/SearchData.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "../../utils/GenericGuard.h"
#include "searchconfig.h"
#include <QMimeData>
#include <QApplication>  // Add QApplication include
#include "../StyleManager.h" // Include StyleManager


///////////////////////////////////////////////////////////////////////////
//                      SearchListWidget implementation
///////////////////////////////////////////////////////////////////////////
SearchListWidget::SearchListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent)
    : QWidget(parent), workspaceId(workspaceId), colorIndex(0), bridge(bridge)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel(tr("Searchs"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #0078d4;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    searchListWidget = new QListWidget(this);
    // Set item height to accommodate the custom widget
    searchListWidget->setStyleSheet("QListWidget::item { min-height: 40px; }");
    
    // Enable drag and drop functionality
    searchListWidget->setDragEnabled(true);
    searchListWidget->setAcceptDrops(true);
    searchListWidget->setDropIndicatorShown(true);
    searchListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    
    // Connect signal for item movement
    connect(searchListWidget->model(), &QAbstractItemModel::rowsMoved,
            this, [this](const QModelIndex&, int start, int end, const QModelIndex&, int destination) {
                // Since we only move one item at a time, start and end will be the same
                this->handleItemMoved(start, destination);
            });
    
    layout->addWidget(searchListWidget);
    
    setLayout(layout);
    // Connect to StyleManager for theme changes
    connect(&StyleManager::instance(), &StyleManager::stylesChanged, 
            this, &SearchListWidget::updateWidgetStyles);
}

void SearchListWidget::initializeWithData(int64_t workspaceId) {
    // Clear existing searchs
    searchList.clear();
    searchListWidget->clear();
    bridge.getSearchListFrmWorkspace(workspaceId, [this](const QList<SearchConfig> &searchs) {
        for (const SearchConfig &search : searchs) {
            assert(static_cast<qint32>(searchList.size()) == search.searchRow);
            searchList.append(search);
            createSearchItem(searchList.size() - 1, search);
        }
    });
} 

void SearchListWidget::addSearch(SearchConfig &search)
{
    // Check if the search string is empty
    if (search.searchPattern.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Search"), tr("Search string cannot be empty."));
        return;
    }

    int32_t searchRow = searchList.size();
    search.searchRow = searchRow;
    int32_t searchId = bridge.addSearchToWorkspace(workspaceId, search);
    if(searchId >= 0) {
        search.searchId = searchId;
        searchList.append(search);
        createSearchItem(searchList.size() - 1, search);
        emit searchsChanged();
    } else {
        QtBridge::getInstance().logError(QString("Failed to add search: %1").arg(search.searchPattern));
    }
}

void SearchListWidget::createSearchItem(int index, const SearchConfig &search)
{
    QListWidgetItem *item = new QListWidgetItem(searchListWidget);
    // Set size to accommodate the custom widget
    item->setSizeHint(QSize(0, 40));
    searchListWidget->addItem(item);
    
    // Create custom widget
    SearchItemWidget *searchItemWidget = new SearchItemWidget(search, index, searchListWidget);
    
    // Connect signals
    connect(searchItemWidget, &SearchItemWidget::searchChanged, this, &SearchListWidget::updateSearch);
    connect(searchItemWidget, &SearchItemWidget::editRequested, this, [this](int idx) {
        searchListWidget->setCurrentRow(idx);
        showEditSearchDialog();
    });
    connect(searchItemWidget, &SearchItemWidget::removeRequested, this, [this](int idx) {
        searchListWidget->setCurrentRow(idx);
        removeSelectedSearch();
    });
    connect(searchItemWidget, &SearchItemWidget::navigateToNextMatch, this, &SearchListWidget::onNavigateToNextMatch);
    connect(searchItemWidget, &SearchItemWidget::navigateToPreviousMatch, this, &SearchListWidget::onNavigateToPreviousMatch);
    
    // Set the custom widget for the item
    searchListWidget->setItemWidget(item, searchItemWidget);
}

void SearchListWidget::updateSearch(int index, const SearchConfig &search)
{
    // Update the search in our list
    if (index >= 0 && index < searchList.size()) {
        searchList[index] = search;
    }
    bridge.updateSearchInWorkspace(workspaceId, search);
    
    // Emit signal that searchs have changed
    emit searchsChanged();
}

void SearchListWidget::doUpdate()
{
    QMap<int, int> matchCounts = bridge.getSearchMatchCounts(workspaceId);
    // Update match counts for each search
    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
        if (widget) {
            if(matchCounts.contains(searchList[i].searchId)){
                widget->updateMatchCount(matchCounts[searchList[i].searchId]);
            }else{
                widget->updateMatchCount(0);
            }
        }
    }
}

void SearchListWidget::onNavigateToNextMatch(int searchId)
{
    emit navigateToNextMatch(searchId);
}

void SearchListWidget::onNavigateToPreviousMatch(int searchId)
{
    emit navigateToPreviousMatch(searchId);
}

QList<SearchConfig> SearchListWidget::getSearchConfigs() const
{
    return searchList;
}

void SearchListWidget::contextMenuEvent(QContextMenuEvent *event)
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
    
    QAction *addAction = menu.addAction(tr("Add Search"));
    
    QListWidgetItem *currentItem = searchListWidget->currentItem();
    QAction *editAction = menu.addAction(tr("Edit Search"));
    QAction *removeAction = menu.addAction(tr("Remove Search"));
    
    // Only enable edit/remove if an item is selected
    bool hasSelection = (currentItem != nullptr);
    editAction->setEnabled(hasSelection);
    removeAction->setEnabled(hasSelection);
    
    QAction *selectedAction = menu.exec(event->globalPos());
    if (selectedAction == addAction) {
        showAddSearchDialog();
    } else if (selectedAction == editAction && hasSelection) {
        showEditSearchDialog();
    } else if (selectedAction == removeAction && hasSelection) {
        removeSelectedSearch();
    }
}

void SearchListWidget::showAddSearchDialog()
{
    SearchConfig search;
    search.color = bridge.getNextSearchColor(workspaceId);
    SearchDialog *dialog = createSearchDialog(tr("Add Search"), search);
    
    if (dialog->exec() == QDialog::Accepted) {
        SearchConfig search = dialog->getSearchConfig();
        addSearch(search);
    }
    
    delete dialog;
}

void SearchListWidget::showEditSearchDialog()
{
    int currentRow = searchListWidget->currentRow();
    if (currentRow < 0 || currentRow >= searchList.size()) {
        return;
    }
    
    SearchConfig currentSearch = searchList.at(currentRow);
    SearchDialog *dialog = createSearchDialog(tr("Edit Search"), currentSearch);
    
    if (dialog->exec() == QDialog::Accepted) {
        SearchConfig updatedSearch = dialog->getSearchConfig();
        
        // Check if the search string is empty
        if (updatedSearch.searchPattern.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Search"), tr("Search string cannot be empty."));
            delete dialog;
            return;
        }
        
        // Update the search in the list
        searchList[currentRow] = updatedSearch;
        
        // Update the widget
        QListWidgetItem *item = searchListWidget->item(currentRow);
        if (item) {
            // Remove the old widget
            QWidget *oldWidget = searchListWidget->itemWidget(item);
            if (oldWidget) {
                searchListWidget->removeItemWidget(item);
                delete oldWidget;
            }
            
            // Create a new widget with updated search
            SearchItemWidget *searchItemWidget = new SearchItemWidget(updatedSearch, currentRow, searchListWidget);
            
            GenericGuard guard(
                [&]() {
                    bridge.beginWorkspaceUpdate();
                    bridge.beginSearchUpdate(workspaceId);
                },
                [&]() {
                    bridge.commitWorkspaceUpdate();
                    bridge.commitSearchUpdate(workspaceId);
                },
                [&]() {
                    bridge.rollbackWorkspaceUpdate();
                    bridge.rollbackSearchUpdate(workspaceId);
                }
            );
            bridge.updateSearchInWorkspace(workspaceId, updatedSearch);
            guard.commit();
            // Connect signals
            connect(searchItemWidget, &SearchItemWidget::searchChanged, this, &SearchListWidget::updateSearch);
            connect(searchItemWidget, &SearchItemWidget::editRequested, this, [this](int idx) {
                searchListWidget->setCurrentRow(idx);
                showEditSearchDialog();
            });
            connect(searchItemWidget, &SearchItemWidget::removeRequested, this, [this](int idx) {
                searchListWidget->setCurrentRow(idx);
                removeSelectedSearch();
            });
            connect(searchItemWidget, &SearchItemWidget::navigateToNextMatch, this, &SearchListWidget::onNavigateToNextMatch);
            connect(searchItemWidget, &SearchItemWidget::navigateToPreviousMatch, this, &SearchListWidget::onNavigateToPreviousMatch);
            
            // Set the custom widget for the item
            searchListWidget->setItemWidget(item, searchItemWidget);
            
            QtBridge::getInstance().logInfo(QString("Updated search: %1").arg(updatedSearch.searchPattern));
            
            // Emit signal that searchs have changed
            emit searchsChanged();
        }
    }
    
    delete dialog;
}

void SearchListWidget::removeSelectedSearch()
{
    int currentRow = searchListWidget->currentRow();
    assert(currentRow >= 0 && currentRow < searchList.size());
    GenericGuard guard(
        [&]() {
            bridge.beginWorkspaceUpdate();
            bridge.beginSearchUpdate(workspaceId);
        },
        [&]() {
            bridge.commitWorkspaceUpdate();
            bridge.commitSearchUpdate(workspaceId);
        },
        [&]() {
            bridge.rollbackWorkspaceUpdate();
            bridge.rollbackSearchUpdate(workspaceId);
        }
    );

    bridge.removeSearchFromWorkspace(workspaceId, searchList[currentRow].searchId);
    searchList.removeAt(currentRow);
    QListWidgetItem *item = searchListWidget->takeItem(currentRow);
    delete item;
    //update the indices of the remaining search widgets

    QList<qint32> searchIds;
    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
        if (widget) {
            widget->setSearchIndex(i);
            searchIds.append(searchList[i].searchId);
        }
    }
    bridge.updateSearchRowsInWorkspace(workspaceId, searchIds);
    guard.commit();
    emit searchsChanged();
}

SearchDialog* SearchListWidget::createSearchDialog(const QString &title, const SearchConfig &initialSearch)
{
    SearchDialog *dialog = new SearchDialog(title, this);
    dialog->setSearchConfig(initialSearch);
    
    return dialog;
}

void SearchListWidget::onSearchRemoved(const QString &searchPattern)
{
    // Find the search in the list
    int indexToRemove = -1;
    for (int i = 0; i < searchList.size(); ++i) {
        if (searchList[i].searchPattern == searchPattern) {
            indexToRemove = i;
            break;
        }
    }

    if (indexToRemove >= 0) {
        // Remove the search from the list
        searchList.removeAt(indexToRemove);

        // Remove the item from the list widget
        QListWidgetItem *item = searchListWidget->takeItem(indexToRemove);
        delete item;

        QtBridge::getInstance().logInfo(QString("Removed search: %1").arg(searchPattern));
        QtBridge::getInstance().troubleshootingLog("SEARCH", "REMOVE", searchPattern);

        // Update the indices of the remaining search widgets
        for (int i = 0; i < searchListWidget->count(); i++) {
            QListWidgetItem *item = searchListWidget->item(i);
            SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
            if (widget) {
                widget->setSearchIndex(i);
            }
        }

        // Emit signal that searchs have changed
        emit searchsChanged();
    }
}

// Helper method to count matches in content
int SearchListWidget::countMatches(const QString &content, const SearchConfig &search)
{
    int count = 0;

    if (search.isRegex) {
        // Use regular expression
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!search.caseSensitive) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }

        QString pattern = search.searchPattern;
        if (search.wholeWord) {
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
        Qt::CaseSensitivity cs = search.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

        if (search.wholeWord) {
            // Split content into words and count matches
            QStringList words = content.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
            for (const QString &word : words) {
                if (word.compare(search.searchPattern, cs) == 0) {
                    count++;
                }
            }
        } else {
            // Count all occurrences
            int pos = 0;
            while ((pos = content.indexOf(search.searchPattern, pos, cs)) != -1) {
                count++;
                pos += search.searchPattern.length();
            }
        }
    }

    return count;
}

// Process searchs against content
void SearchListWidget::applySearchs(const QString &content)
{
    // Process each search
    for (int i = 0; i < searchList.size(); ++i) {
        SearchConfig &search = searchList[i];

        // Skip disabled searchs
        if (!search.enabled) {
            continue;
        }

        // Count matches
        int matchCount = countMatches(content, search);
        search.matchCount = matchCount;

        // Update the UI item
        QListWidgetItem *item = searchListWidget->item(i);
        if (item) {
            SearchItemWidget *searchItemWidget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
            if (searchItemWidget) {
                searchItemWidget->updateMatchCount(matchCount);
            }
        }
    }

    // Emit signal that searchs have been processed
    emit searchsChanged();
}

void SearchListWidget::handleItemMoved(int fromRow, int toRow)
{
    updateSearchRows();
    emit searchsChanged();
}

void SearchListWidget::updateSearchRows()
{
    GenericGuard guard(
        [&]() {
            bridge.beginWorkspaceUpdate();
            bridge.beginSearchUpdate(workspaceId);
        },
        [&]() {
            bridge.commitWorkspaceUpdate();
            bridge.commitSearchUpdate(workspaceId);
        },
        [&]() {
            bridge.rollbackWorkspaceUpdate();
            bridge.rollbackSearchUpdate(workspaceId);
        }
    );
    searchList.clear();
    QList<qint32> searchIds;
    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        if (item) {
            SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
            if (widget) {
                widget->setSearchIndex(i);
                SearchConfig search = widget->getSearchConfig();
                searchList.append(search);
                searchIds.append(search.searchId);
            }
        }
    }
    QtBridge::getInstance().updateSearchRowsInWorkspace(workspaceId, searchIds);
    guard.commit();
}

void SearchListWidget::updateWidgetStyles()
{
    // Update styles for all filter item widgets
    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
        if (widget) {
            widget->applySystemStyles();
        }
    }
}

///////////////////////////////////////////////////////////////////////////
//                      SearchDialog implementation
///////////////////////////////////////////////////////////////////////////
SearchDialog::SearchDialog(const QString &title, QWidget *parent)
    : QDialog(parent), currentColor(QColor("#FF4444"))  // Default to red
{
    setWindowTitle(title);
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout for the search properties
    QFormLayout *formLayout = new QFormLayout();

    // Search string input
    searchStringEdit = new QLineEdit(this);
    searchStringEdit->setMinimumWidth(400);
    formLayout->addRow(tr("Search:"), searchStringEdit);

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
    connect(colorButton, &QPushButton::clicked, this, &SearchDialog::selectColor);

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
        if (searchStringEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Search string cannot be empty."));
            searchStringEdit->setFocus();
            reject();
        }
    });

    setLayout(mainLayout);
}

void SearchDialog::updateColorButton()
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

void SearchDialog::setSearchConfig(const SearchConfig &search)
{
    m_searchId = search.searchId;
    m_searchRow = search.searchRow;
    searchStringEdit->setText(search.searchPattern);
    enabledCheckBox->setChecked(search.enabled);
    caseSensitiveCheckBox->setChecked(search.caseSensitive);
    wholeWordCheckBox->setChecked(search.wholeWord);
    regexCheckBox->setChecked(search.isRegex);
    currentColor = search.color;
    updateColorButton();
}

SearchConfig SearchDialog::getSearchConfig() const
{
    return SearchConfig(
        m_searchId,
        m_searchRow,
        searchStringEdit->text(),
        caseSensitiveCheckBox->isChecked(),
        wholeWordCheckBox->isChecked(),
        regexCheckBox->isChecked(),
        enabledCheckBox->isChecked(),
        currentColor
    );
}

void SearchDialog::selectColor()
{
    QColor color = QColorDialog::getColor(currentColor, this, tr("Select Search Color"));

    if (color.isValid()) {
        currentColor = color;
        updateColorButton();
    }
}

///////////////////////////////////////////////////////////////////////////
//                      SearchItemWidget implementation
///////////////////////////////////////////////////////////////////////////

SearchItemWidget::SearchItemWidget(const SearchConfig &search, int index, QWidget *parent)
    : QWidget(parent), currentSearch(search), itemIndex(index)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    
    // Enable checkbox
    enabledButton = new QCheckBox("", this);
    enabledButton->setChecked(search.enabled);
    enabledButton->setToolTip(tr("Enable/Disable Search"));
    connect(enabledButton, &QCheckBox::toggled, this, &SearchItemWidget::onEnabledToggled);
    layout->addWidget(enabledButton);
    
    // Search text label
    searchLabel = new QLabel(search.searchPattern, this);
    searchLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(search.color.name()));
    searchLabel->setCursor(Qt::PointingHandCursor);
    searchLabel->setToolTip(search.searchPattern);
    searchLabel->setWordWrap(false);
    searchLabel->setTextFormat(Qt::PlainText);
    
    layout->addWidget(searchLabel, 1);
    
    // Match count label
    matchCountLabel = new QLabel(tr("Matches: 0"), this);
    matchCountLabel->setAlignment(Qt::AlignCenter);
    matchCountLabel->setMinimumWidth(80);
    layout->addWidget(matchCountLabel);
    
    // Navigation buttons
    prevMatchButton = new QPushButton("◀", this);
    prevMatchButton->setFixedSize(24, 24);
    prevMatchButton->setToolTip(tr("Previous Match"));
    prevMatchButton->setEnabled(false);
    connect(prevMatchButton, &QPushButton::clicked, this, &SearchItemWidget::onPrevMatchClicked);
    layout->addWidget(prevMatchButton);
    
    nextMatchButton = new QPushButton("▶", this);
    nextMatchButton->setFixedSize(24, 24);
    nextMatchButton->setToolTip(tr("Next Match"));
    nextMatchButton->setEnabled(false);
    connect(nextMatchButton, &QPushButton::clicked, this, &SearchItemWidget::onNextMatchClicked);
    layout->addWidget(nextMatchButton);
    
    // Case sensitive checkbox
    caseSensitiveButton = new QToolButton(this);
    caseSensitiveButton->setText("Aa");
    caseSensitiveButton->setCheckable(true);
    caseSensitiveButton->setChecked(search.caseSensitive);
    caseSensitiveButton->setToolTip(tr("Case Sensitive"));
    caseSensitiveButton->setFixedSize(24, 24);
    connect(caseSensitiveButton, &QToolButton::toggled, this, &SearchItemWidget::onCaseSensitiveToggled);
    layout->addWidget(caseSensitiveButton);
    
    // Whole word checkbox
    wholeWordButton = new QToolButton(this);
    wholeWordButton->setText("ab");
    wholeWordButton->setCheckable(true);
    wholeWordButton->setChecked(search.wholeWord);
    wholeWordButton->setToolTip(tr("Whole Word"));
    wholeWordButton->setFixedSize(24, 24);
    connect(wholeWordButton, &QToolButton::toggled, this, &SearchItemWidget::onWholeWordToggled);
    layout->addWidget(wholeWordButton);
    
    // Regex checkbox
    regexButton = new QToolButton(this);
    regexButton->setText(".*");
    regexButton->setCheckable(true);
    regexButton->setChecked(search.isRegex);
    regexButton->setToolTip(tr("Regular Expression"));
    regexButton->setFixedSize(24, 24);
    connect(regexButton, &QToolButton::toggled, this, &SearchItemWidget::onRegexToggled);
    layout->addWidget(regexButton);
    
    // Color button
    colorButton = new QPushButton(this);
    colorButton->setFixedSize(24, 24);
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(search.color.name()));
    connect(colorButton, &QPushButton::clicked, this, &SearchItemWidget::onColorClicked);
    layout->addWidget(colorButton);
    
    // Remove button
    removeButton = new QPushButton(QIcon::fromTheme("edit-delete"), "", this);
    removeButton->setFixedSize(24, 24);
    removeButton->setToolTip(tr("Remove Search"));
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeRequested(itemIndex);
    });
    layout->addWidget(removeButton);
  
    setLayout(layout);
    
    // Connect double-click on the search label to edit
    connect(searchLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit editRequested(itemIndex);
    });
    
    // Install event filter to handle mouse events
    searchLabel->installEventFilter(this);

    // Apply system theme styles
    applySystemStyles();
}

// Add event filter to handle double-click on search label
bool SearchItemWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == searchLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            emit editRequested(itemIndex);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SearchItemWidget::updateMatchCount(int count)
{
    currentSearch.matchCount = count;
    matchCountLabel->setText(tr("Matches: %1").arg(count));
    
    // Enable/disable navigation buttons based on match count
    bool hasMatches = (count > 0);
    prevMatchButton->setEnabled(hasMatches);
    nextMatchButton->setEnabled(hasMatches);
    
    // Update tooltip to show current position if there are matches
    if (hasMatches) {
        int position = currentSearch.currentMatchPosition + 1;
        // Ensure position is within valid range
        if (position < 1) position = 1;
        if (position > count) position = count;
        
        matchCountLabel->setToolTip(tr("Match %1 of %2").arg(position).arg(count));
    } else {
        matchCountLabel->setToolTip(tr("No matches found"));
        // Reset current position when no matches
        currentSearch.currentMatchPosition = 0;
    }
}

void SearchItemWidget::setSearchIndex(int index) {
    itemIndex = index;
    currentSearch.searchRow = index;
}

void SearchItemWidget::onNextMatchClicked()
{
    emit navigateToNextMatch(currentSearch.searchId);
}

void SearchItemWidget::onPrevMatchClicked()
{
    emit navigateToPreviousMatch(currentSearch.searchId);
}

void SearchItemWidget::onEnabledToggled(bool checked)
{
    currentSearch.enabled = checked;
    updateEnabledState();
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::updateEnabledState()
{
    bool isEnabled = currentSearch.enabled;
    
    QString styleSheet = QString("color: %1; font-weight: bold;").arg(currentSearch.color.name());
    if (!isEnabled) {
        styleSheet += " opacity: 0.5;";
    }
    searchLabel->setStyleSheet(styleSheet);
    
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

void SearchItemWidget::onCaseSensitiveToggled(bool checked)
{
    currentSearch.caseSensitive = checked;
    updateCaseSensitiveButtonStyle();
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::onWholeWordToggled(bool checked)
{
    currentSearch.wholeWord = checked;
    updateWholeWordButtonStyle();
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::onRegexToggled(bool checked)
{
    currentSearch.isRegex = checked;
    updateRegexButtonStyle();
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::onColorClicked()
{
    QColor color = QColorDialog::getColor(currentSearch.color, this, tr("Select Search Color"));
    
    if (color.isValid()) {
        currentSearch.color = color;
        colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(color.name()));
        searchLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color.name()));
        emit searchChanged(itemIndex, currentSearch);
    }
}

// Add this method to the SearchItemWidget class
void SearchItemWidget::setSearchConfig(const SearchConfig &search)
{
    currentSearch = search;
    itemIndex = search.searchRow; // Update the item index to match the search row
    
    searchLabel->setText(search.searchPattern);
    searchLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(search.color.name()));
    searchLabel->setToolTip(search.searchPattern);
    
    // Update button states
    enabledButton->setChecked(search.enabled);
    caseSensitiveButton->setChecked(search.caseSensitive);
    wholeWordButton->setChecked(search.wholeWord);
    regexButton->setChecked(search.isRegex);
    
    // Update color button
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(search.color.name()));
    
    // Update match count
    updateMatchCount(search.matchCount);
    
    // Update enabled state
    updateEnabledState();
}

void SearchItemWidget::updateSearchStyle()
{
    QString styleSheet = QString("color: %1; font-weight: bold;").arg(currentSearch.color.name());
    searchLabel->setStyleSheet(styleSheet);
    colorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #888;").arg(currentSearch.color.name()));
}

void SearchItemWidget::showEditDialog()
{
    SearchDialog *dialog = new SearchDialog(tr("Edit Search"), this);
    dialog->setSearchConfig(currentSearch);
    
    if (dialog->exec() == QDialog::Accepted) {
        // Get updated search configuration
        SearchConfig updatedSearch = dialog->getSearchConfig();
        
        // Check if the search string is empty
        if (updatedSearch.searchPattern.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Search"), tr("Search string cannot be empty."));
            delete dialog;
            return;
        }
        
        // Update current search
        currentSearch = updatedSearch;
        
        // Update UI
        setSearchConfig(currentSearch);
        
        // Log the update
        QtBridge::getInstance().logInfo(QString("Updated search: %1").arg(updatedSearch.searchPattern));
        
        // Emit signal that search has changed
        emit searchChanged(itemIndex, currentSearch);
    }
    
    delete dialog;
}

void SearchItemWidget::updateCaseSensitiveButtonStyle()
{
    caseSensitiveButton->setStyleSheet(StyleManager::instance().getFilterSearchToolButtonStyle(caseSensitiveButton->isChecked()));
}

void SearchItemWidget::updateWholeWordButtonStyle()
{
    QString styleSheet = StyleManager::instance().getFilterSearchToolButtonStyle(wholeWordButton->isChecked());
    // Add text decoration for the whole word button
    styleSheet += " QToolButton { text-decoration: underline; }";
    wholeWordButton->setStyleSheet(styleSheet);
}

void SearchItemWidget::updateRegexButtonStyle()
{
    regexButton->setStyleSheet(StyleManager::instance().getFilterSearchToolButtonStyle(regexButton->isChecked()));
}

void SearchItemWidget::applySystemStyles()
{
    // Get styles from StyleManager
    const StyleManager& styleManager = StyleManager::instance();
    
    // Apply navigation button styles
    prevMatchButton->setStyleSheet(styleManager.getFilterSearchNavigationButtonStyle());
    nextMatchButton->setStyleSheet(styleManager.getFilterSearchNavigationButtonStyle());
    
    // Apply tool button styles based on their checked state
    updateCaseSensitiveButtonStyle();
    updateWholeWordButtonStyle();
    updateRegexButtonStyle();
    
    // Apply match count label style
    matchCountLabel->setStyleSheet(styleManager.getMatchCountLabelStyle());
    
    // Keep filter label color unchanged, as it's part of the filter identity
    // Update colorButton, but keep its background color as the filter color
    QString buttonStyleBase = styleManager.getFilterSearchNavigationButtonStyle();
    QString colorButtonStyle = QString("background-color: %1; border: 1px solid #888;").arg(currentSearch.color.name());
    colorButton->setStyleSheet(colorButtonStyle);
    
    // Apply navigation button style to removeButton
    removeButton->setStyleSheet(styleManager.getFilterSearchNavigationButtonStyle());
    
    // Update enabled state to refresh all widgets
    updateEnabledState();
}
