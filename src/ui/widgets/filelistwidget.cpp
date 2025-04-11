#include "filelistwidget.h"
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include "../../core/TimeUtils.h"
#include "../../core/FileSystem.h"
#include "../../bridge/QtBridge.h"
#include "../../utils/GenericGuard.h"

FileListWidget::FileListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent)
    : workspaceId(workspaceId),bridge(bridge), QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Create header
    QLabel *headerLabel = new QLabel(tr("Files"), this);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #0078d4;");
    headerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(headerLabel);
    
    // Create file list widget
    fileListWidget = new QListWidget(this);
    fileListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    fileListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    fileListWidget->setDefaultDropAction(Qt::MoveAction);
    fileListWidget->setAcceptDrops(true);
    fileListWidget->viewport()->installEventFilter(this);
    fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    fileListWidget->setSpacing(0);
    fileListWidget->setUniformItemSizes(true);
    fileListWidget->setStyleSheet("QListWidget::item { min-height: 40px; }");
    
    // Connect signals for item position changes
    connect(fileListWidget->model(), &QAbstractItemModel::rowsMoved, 
            this, [this](const QModelIndex&, int start, int, const QModelIndex&, int destination) {
        handleItemMoved(start, destination);
    });
    
    // Connect signals
    connect(fileListWidget, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
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
        
        QAction *addAction = contextMenu.addAction(tr("Add File"));
        QAction *removeAction = contextMenu.addAction(tr("Remove Selected"));
        
        connect(addAction, &QAction::triggered, this, [this]() {
            addFilesFromDialog();
        });
        
        connect(removeAction, &QAction::triggered, this, [this]() {
            QList<QListWidgetItem*> selectedItems = fileListWidget->selectedItems();
            if (!selectedItems.isEmpty()) {
                int index = fileListWidget->row(selectedItems.first());
                removeFile(index);
            }
        });
        
        contextMenu.exec(fileListWidget->mapToGlobal(pos));
    });
    
    // Set up drag and drop
    setAcceptDrops(true);
    
    layout->addWidget(fileListWidget);
    
    setLayout(layout);
}

void FileListWidget::initializeWithData(int64_t workspaceId)
{
    // Load file list from workspace
    this->fileList.clear();
    fileListWidget->clear();
    bridge.getFileListFromWorkspace(workspaceId, [this](const QList<FileInfo> &fileList) {
        for (const FileInfo &fileInfo : fileList) {
            assert(static_cast<qint32>(this->fileList.size()) == fileInfo.fileRow);
            this->fileList.append(fileInfo);
            createFileItem(this->fileList.size()-1, fileInfo);
        }
    });
}

bool FileListWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == fileListWidget->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QListWidgetItem *item = fileListWidget->itemAt(mouseEvent->pos());
            if (item) {
                // Select the item when clicked directly on it
                fileListWidget->setCurrentItem(item);
                
                // Pass the event to the QListWidget to handle drag initiation
                return false;
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}
void FileListWidget::addFile(QString filePath)
{
    QtBridge::getInstance().logInfo("Enter FileListWidget::addFile");
    if(filePath.isEmpty()) {
        QtBridge::getInstance().logWarning("Attempted to add file with empty path. Ignoring.");
        return;
    }
    //linke the row of the new added file to FileData in Core
    int32_t fileRow = fileList.size();
    int32_t fileId = bridge.addFileToWorkspace(workspaceId, fileRow, filePath);
    if(fileId >= 0) {
        FileInfo fileInfo;
        if(bridge.getFileInfoFromWorkspace(workspaceId, fileId, fileInfo)) {
            // Add to internal list
            fileList.append(fileInfo);
            
            // Create the file item with widget
            createFileItem(fileList.size() - 1, fileInfo);
            
            // Log the addition
            QtBridge::getInstance().logInfo(QString("FileListWidget:addFile Added file to list: %1 (index: %2)")
                                          .arg(fileInfo.filePath)
                                          .arg(fileInfo.fileId));
            
            // Emit signals to notify that file list has changed
            emit filesChanged();
        }
    } else {
        QtBridge::getInstance().logError(QString("FileListWidget::addFile Failed to add file to workspace: %1").arg(filePath));
    }
}

void FileListWidget::createFileItem(int index, const FileInfo &fileInfo) {
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 40)); // Match the min-height of QListWidget::item
    fileListWidget->addItem(item);
    
    FileItemWidget *fileItemWidget = new FileItemWidget(fileInfo, this);
    connect(fileItemWidget, &FileItemWidget::selectionChanged, this, &FileListWidget::updateFileSelection);
    connect(fileItemWidget, &FileItemWidget::removeRequested, this, &FileListWidget::removeFile);
    
    fileListWidget->setItemWidget(item, fileItemWidget);
}

QList<FileInfo> FileListWidget::getSelectedFiles() const {
    QList<FileInfo> selectedFiles;
    for (int i = 0; i < fileList.size(); ++i) {
        QListWidgetItem *item = fileListWidget->item(i);
        FileItemWidget *widget = qobject_cast<FileItemWidget*>(fileListWidget->itemWidget(item));
        if (widget && widget->isSelected()) {
            selectedFiles.append(fileList[i]);
        }
    }
    return selectedFiles;
}

void FileListWidget::updateFileSelection(int id, bool selected) {
    for(auto &file : fileList) {
        if(file.fileId == id) {
            file.isSelected = selected;
            if(workspaceId >= 0) {
                QtBridge::getInstance().updateFileSelectionInWorkspace(workspaceId, file.fileId, selected);
            }
            emit filesChanged();
            break;
        }
    }
}

void FileListWidget::removeFile(qint32 index) {
    QtBridge::getInstance().logInfo("Enter FileListWidget::removeFile");
    assert(index >= 0 && index < fileList.size());
    // Get file info for logging
    int32_t fileId = fileList[index].fileId;
    QString filePath = fileList[index].filePath;
    QtBridge::getInstance().removeFileFromWorkspace(workspaceId, fileId);
    fileList.removeAt(index);
    QListWidgetItem *item = fileListWidget->takeItem(index);
    delete item;
    
    // Update the indices of the remaining files
    updateAllFileIndices();
    
    emit filesChanged();
    QtBridge::getInstance().logInfo(QString("Removed file: %1 (ID: %2)").arg(filePath).arg(fileId));
}

// Implement drag enter event
void FileListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept internal moves for reordering
    if (event->source() == fileListWidget) {
        event->acceptProposedAction();
        return;
    }

    // Handle external file drops
    if (event->mimeData()->hasUrls()) {
        bool hasValidFile = false;
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                std::filesystem::path fsPath = Core::PathConverter::fromString(
                    url.toLocalFile().toStdString());
                if (Core::FileSystem::isRegularFile(fsPath)) {
                    hasValidFile = true;
                    break;
                }
            }
        }
        if (hasValidFile) {
            event->acceptProposedAction();
        }
    }
}

// Implement drop event
void FileListWidget::dropEvent(QDropEvent *event)
{
    // Let the QListWidget handle internal moves
    if (event->source() == fileListWidget) {
        event->acceptProposedAction();
        return; // The rowsMoved signal will handle the reordering
    }

    // Handle external file drops
    if (event->mimeData()->hasUrls()) {
        addFilesFromUrls(event->mimeData()->urls());
        event->acceptProposedAction();
    }
}

// Helper method to add files from URLs
void FileListWidget::addFilesFromUrls(const QList<QUrl> &urls)
{
    if (workspaceId < 0) {
        QtBridge::getInstance().logWarning("Cannot add files: Invalid workspace index");
        return;
    }

    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            std::filesystem::path fsPath = Core::PathConverter::fromString(filePath.toStdString());
            
            if (Core::FileSystem::isRegularFile(fsPath)) {
                addFile(filePath);
            } else {
                QtBridge::getInstance().logWarning(QString("Skipped non-file item: %1").arg(filePath));
            }
        }
    }
}

QString FileListWidget::getFilePathAt(int index) const
{
    if (index >= 0 && index < fileList.size()) {
        return fileList[index].filePath;
    }
    return QString();
}

int FileListWidget::findFileIndex(const QString &filePath) const
{
    for (int i = 0; i < fileList.size(); ++i) {
        if (fileList[i].filePath == filePath) {
            return i;
        }
    }
    return -1;
}

void FileListWidget::addFilesFromDialog()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        tr("Select Log Files"),
        QDir::homePath(),
        tr("All Files (*)")
    );
    
    if (!filePaths.isEmpty()) {
        for (const QString &filePath : filePaths) {
            addFile(filePath);
        }
    }
}

QString FileListWidget::formatFileSize(qint64 size) {
    if (size < 1024) {
        return QString::number(size) + " B";
    } else if (size < 1024 * 1024) {
        return QString::number(size / 1024) + " KB";
    } else if (size < 1024 * 1024 * 1024) {
        return QString::number(size / (1024 * 1024)) + " MB";
    } else {
        return QString::number(size / (1024 * 1024 * 1024)) + " GB";
    }
}

// FileItemWidget implementation
FileItemWidget::FileItemWidget(FileInfo fileInfo, QWidget *parent)
    : m_fileInfo(fileInfo), QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);
    
    // Set a fixed height for the widget
    setFixedHeight(30);
    
    // Index label (showing fileIndex which starts from 1)
    indexLabel = new QLabel(QString::number(fileInfo.fileId), this);
    indexLabel->setStyleSheet("font-weight: bold; min-width: 20px;");
    indexLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(indexLabel);
    
    // Checkbox for selection
    checkBox = new QCheckBox(this);
    checkBox->setChecked(fileInfo.isSelected);
    connect(checkBox, &QCheckBox::toggled, this, &FileItemWidget::onCheckBoxToggled);
    layout->addWidget(checkBox);
    
    // File name label with existence check
    QString displayText = fileInfo.fileName;
    if (!fileInfo.isExists) {
        displayText += " (File does not exist)";
    }
    fileNameLabel = new QLabel(displayText, this);
    QString labelStyle = "padding-left: 5px;";
    if (!fileInfo.isExists) {
        labelStyle += " color: red;";
    }
    fileNameLabel->setStyleSheet(labelStyle);
    fileNameLabel->setToolTip(fileInfo.filePath);
    layout->addWidget(fileNameLabel, 1); // Stretch factor 1
    
    // Remove button
    removeButton = new QPushButton(QIcon::fromTheme("edit-delete"), "", this);
    removeButton->setFixedSize(24, 24);
    removeButton->setToolTip(tr("Remove File"));
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeRequested(m_fileInfo.fileRow);
    });
    layout->addWidget(removeButton);
    
    setLayout(layout);
    
    // Make the widget clickable
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    
    // 为整个widget也设置tooltip，这样鼠标悬停在任何位置都能看到完整路径
    setToolTip(fileInfo.filePath);
}

void FileItemWidget::onCheckBoxToggled(bool checked) {
    emit selectionChanged(m_fileInfo.fileId, checked);
}

// Override mouse press event to emit clicked signal
void FileItemWidget::mousePressEvent(QMouseEvent *event) {
    emit clicked(m_fileInfo.fileId);
    QWidget::mousePressEvent(event);
}

void FileItemWidget::setFileIndex(int index) {
    m_fileInfo.fileRow = index;
    indexLabel->setText(QString::number(index + 1)); // Display 1-based index
}

// Handle item movement (drag and drop)
void FileListWidget::handleItemMoved(int fromIndex, int toIndex) {
    QtBridge::getInstance().logInfo(QString("FileListWidget::handleItemMoved - Item moved from %1 to %2").arg(fromIndex).arg(toIndex));

    if (fromIndex < 0 || fromIndex >= fileList.size() || toIndex < 0 || toIndex >= fileList.size()) {
        QtBridge::getInstance().logError("Invalid index for item move operation");
        return;
    }

    // Move the item in the internal list
    FileInfo movedFileInfo = fileList.takeAt(fromIndex);
    fileList.insert(toIndex, movedFileInfo);

    // Update file indices in the database
    updateAllFileIndices();

    // Notify about the change
    emit filesChanged();
}

// Update all file indices in the database and UI
void FileListWidget::updateAllFileIndices() {
    GenericGuard guard(
        [&]() {
            bridge.beginFileUpdate(workspaceId);
        },
        [&]() {
            bridge.commitFileUpdate(workspaceId);
        },
        [&]() {
            bridge.rollbackFileUpdate(workspaceId);
        }
    );
    for (int i = 0; i < fileListWidget->count(); i++) {
        QListWidgetItem *item = fileListWidget->item(i);
        FileItemWidget *widget = qobject_cast<FileItemWidget*>(fileListWidget->itemWidget(item));
        
        if (widget) {
            widget->setFileIndex(i);
            // Update the row in the database
            fileList[i].fileRow = i;
            QtBridge::getInstance().updateFileRowInWorkspace(workspaceId, fileList[i].fileId, i);
        }
    }
    guard.commit();
}