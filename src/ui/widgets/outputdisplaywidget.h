#ifndef OUTPUTDISPLAYWIDGET_H
#define OUTPUTDISPLAYWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include "qoutputline.h"

#include <vector>
#include <string>

class QtBridge;

// Custom class for displaying line numbers and file info
class InfoAreaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InfoAreaWidget(QTextEdit *editor);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    
    void setLineInfos(const QStringList &lineInfos);

private:
    QTextEdit *textEditor;
    QStringList lineInfos;
};

class OutputDisplayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OutputDisplayWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent = nullptr);
    ~OutputDisplayWidget();

    void clearDisplay();
    void doUpdate();
    void onNavigateToNextFilterMatch(int filterId);
    void onNavigateToPreviousFilterMatch(int filterId);
    void onNavigateToNextSearchMatch(int searchId);
    void onNavigateToPreviousSearchMatch(int searchId);
    
signals:
    void titleChanged(const QString &title);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onSystemThemeChanged(const QPalette &palette);
    void onScrollBarMoved(int value);

private:
    void setupTextEdit();
    void updateInfoArea();
    void updateDisplay(int startLine, int lineCount);
    void applyHighlighting();
    QString formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const;
    int getLineStartPosition(int lineIndex) const;

    QLabel *headerLabel;
    QWidget *innerWidget;
    InfoAreaWidget *infoArea;
    QTextEdit *textEditLines;
    QtBridge& bridge;
    int64_t workspaceId;
    QList<QOutputLine> outputLines; // Store all lines
    QStringList currentLineInfos;
    int visibleLines; // Number of lines visible in viewport
    static constexpr int CHUNK_SIZE = 10000; // Chunk size for loading
    bool isUpdatingDisplay; // 防止递归调用的标志
};

#endif // OUTPUTDISPLAYWIDGET_H