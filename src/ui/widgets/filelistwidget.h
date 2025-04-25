#ifndef FILELISTWIDGET_H
#define FILELISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QFileDialog>
#include <QCheckBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "../bridge/QtBridge.h"
#include "../models/fileinfo.h"

class FileItemWidget;

class FileListWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileListWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent = nullptr);
    void initializeWithData(int64_t workspaceId);

    void addFile(QString filePath);
    QList<FileInfo> getSelectedFiles() const;
    QString getFilePathAt(int index) const;
    int findFileIndex(const QString &filePath) const;
    void removeFile(qint32 index);

signals:
    void filesChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void updateFileSelection(int id, bool selected);
    void handleItemMoved(int fromIndex, int toIndex);

private:
    int64_t workspaceId = -1;
    QtBridge& bridge;
    QListWidget *fileListWidget;
    QList<FileInfo> fileList;
    
    void createFileItem(int index, const FileInfo &fileInfo);
    void addFilesFromUrls(const QList<QUrl> &urls);
    void addFilesFromDialog();
    QString formatFileSize(qint64 size);
    void updateAllFileIndices();
};

// Custom widget for file items
class FileItemWidget : public QWidget {
    Q_OBJECT
public:
    FileItemWidget(FileInfo fileInfo, QWidget *parent = nullptr);
    bool isSelected() const { return checkBox->isChecked(); }
    int32_t getFileId() const { return m_fileInfo.fileId; }
    void setFileIndex(int index);
    FileInfo getFileInfo() const { return m_fileInfo; }

signals:
    void selectionChanged(int id, bool selected);
    void clicked(int id);
    void removeRequested(int id);

private slots:
    void onCheckBoxToggled(bool checked);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    FileInfo m_fileInfo;
    QLabel *indexLabel;
    QLabel *fileNameLabel;
    QCheckBox *checkBox;
    QPushButton *removeButton;
};

#endif // FILELISTWIDGET_H 