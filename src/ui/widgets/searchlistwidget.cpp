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
#include <QMimeData>
// Define a list of readable, distinct colors
const QList<QColor> SearchListWidget::predefinedColors = {
    QColor("#FF0000"), // Red
    QColor("#0000FF"), // Blue
    QColor("#008000"), // Green
    QColor("#800080"), // Purple
    QColor("#FF8C00"), // Dark Orange
    QColor("#1E90FF"), // Dodger Blue
    QColor("#FF1493"), // Deep Pink
    QColor("#00CED1"), // Dark Turquoise
    QColor("#8B4513"), // Saddle Brown
    QColor("#2E8B57"), // Sea Green
    QColor("#4B0082"), // Indigo
    QColor("#800000"), // Maroon
    QColor("#708090"), // Slate Gray
    QColor("#000080"), // Navy
    QColor("#8B008B"), // Dark Magenta
    QColor("#556B2F")  // Dark Olive Green
};

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

QColor SearchListWidget::getNextColor()
{
    if (predefinedColors.isEmpty()) {
        return QColor("#FF0000"); // Default to red if no colors defined
    }
    
    // Get a list of colors already in use
    QList<QColor> usedColors;
    for (const SearchConfig &search : std::as_const(searchList)) {
        usedColors.append(search.color);
    }
    
    // Find the first color that's not in use
    for (const QColor &color : predefinedColors) {
        if (!usedColors.contains(color)) {
            return color;
        }
    }
    
    // If all colors are in use, return the next color in sequence
    QColor color = predefinedColors[colorIndex % predefinedColors.size()];
    colorIndex++;
    return color;
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
    SearchDialog *dialog = createSearchDialog(tr("Add Search"));
    
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

    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
        if (widget) {
            widget->setSearchIndex(i);
            bridge.updateSearchRowInWorkspace(workspaceId, searchList[i].searchId, i);
        }
    }
    guard.commit();
    emit searchsChanged();
}

SearchDialog* SearchListWidget::createSearchDialog(const QString &title, const SearchConfig &initialSearch)
{
    SearchDialog *dialog = new SearchDialog(title, this);
    
    if (!initialSearch.searchPattern.isEmpty()) {
        dialog->setSearchConfig(initialSearch);
    }
    
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

// SearchDialog implementation
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

// SearchItemWidget implementation
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
    caseSensitiveButton = new QCheckBox("Case", this);
    caseSensitiveButton->setChecked(search.caseSensitive);
    connect(caseSensitiveButton, &QCheckBox::toggled, this, &SearchItemWidget::onCaseSensitiveToggled);
    layout->addWidget(caseSensitiveButton);
    
    // Whole word checkbox
    wholeWordButton = new QCheckBox("Word", this);
    wholeWordButton->setChecked(search.wholeWord);
    connect(wholeWordButton, &QCheckBox::toggled, this, &SearchItemWidget::onWholeWordToggled);
    layout->addWidget(wholeWordButton);
    
    // Regex checkbox
    regexButton = new QCheckBox("Regex", this);
    regexButton->setChecked(search.isRegex);
    connect(regexButton, &QCheckBox::toggled, this, &SearchItemWidget::onRegexToggled);
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
    
    // Update UI based on enabled state
    updateEnabledState();
    
    setLayout(layout);
    
    // Connect double-click on the search label to edit
    connect(searchLabel, &QLabel::linkActivated, this, [this](const QString &) {
        emit editRequested(itemIndex);
    });
    
    // Install event filter to handle mouse events
    searchLabel->installEventFilter(this);
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
    wholeWordButton->setEnabled(isEnabled);
    regexButton->setEnabled(isEnabled);
    colorButton->setEnabled(isEnabled);
}

void SearchItemWidget::onCaseSensitiveToggled(bool checked)
{
    currentSearch.caseSensitive = checked;
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::onWholeWordToggled(bool checked)
{
    currentSearch.wholeWord = checked;
    emit searchChanged(itemIndex, currentSearch);
}

void SearchItemWidget::onRegexToggled(bool checked)
{
    currentSearch.isRegex = checked;
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

void SearchListWidget::handleItemMoved(int fromRow, int toRow)
{
    // Ensure the rows are valid
    if (fromRow < 0 || fromRow >= searchList.size() || 
        toRow < 0 || toRow >= searchList.size() || 
        fromRow == toRow) {
        return;
    }
    
    // Move the search in our internal list
    SearchConfig movedSearch = searchList.takeAt(fromRow);
    searchList.insert(toRow, movedSearch);
    
    // Update search rows for all searchs
    updateSearchRows();
    
    // Notify that searchs have changed
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
    for (int i = 0; i < searchListWidget->count(); i++) {
        QListWidgetItem *item = searchListWidget->item(i);
        if (item) {
            SearchItemWidget *widget = qobject_cast<SearchItemWidget*>(searchListWidget->itemWidget(item));
            if (widget) {
                widget->setSearchIndex(i);
                searchList[i].searchRow = i;
                QtBridge::getInstance().updateSearchRowInWorkspace(workspaceId, searchList[i].searchId, i);
            }
        }
    }
    guard.commit();
}
