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

struct OutputLineInfo
{
    int outputLineNumber;
    int fileIndex;
    int lineIndex;
};

// Custom class for displaying line numbers and file info
class InfoAreaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InfoAreaWidget(QtBridge& bridge, QTextEdit *editor);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    
    void setLineInfoList(int maxFileLineCount, const QVector<OutputLineInfo> &lineInfoList);
    void setLineRange(int startLine, int endLine);
protected:
    QString formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const;
    void updateWidth();
private:
    QtBridge& m_bridge;
    QTextEdit *textEditor = nullptr;
    QStringList lineInfos;
    QVector<OutputLineInfo> m_lineInfoList;
    int m_outputLineFieldWidth = 0;
    int m_fileIndexFieldWidth = 0;
    int m_lineIndexFieldWidth = 0;
    int m_startLine = 0;
    int m_endLine = 0;
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
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSystemThemeChanged(const QPalette &palette);
    void onScrollBarMoved(int value);

private:
    void setupTextEdit();
    QFont getOptimalMonoFont();
    void updateDisplay(int startLine, int lineCount, int matchLineIndex = -1, int matchCharStartIndex = -1, int matchCharEndIndex = -1);
    void updateScrollBarRanges(); // 更新自定义滚动条范围
    QString formatLinePrefix(int outputLineIndex, int outputLineFieldWidth, int fileIndex, int lineIndex) const;
    int getLineStartPosition(int lineIndex) const;

    QLabel *headerLabel = nullptr;
    QWidget *containerWidget = nullptr; // 容器小部件
    QScrollBar *customVerticalScrollBar = nullptr;   // 自定义垂直滚动条
    QScrollBar *customHorizontalScrollBar = nullptr; // 自定义水平滚动条
    
    QWidget *contentWidget = nullptr;
    qreal m_oneLineHeight = 0.0;;
    QWidget *innerWidget = nullptr;
    InfoAreaWidget *infoArea = nullptr;
    QTextEdit *textEditLines = nullptr;
    int m_textEditLinesStartLine = 0;
    int m_textEditLinesEndLine = 0;

    QtBridge& bridge;
    int64_t workspaceId;
    QList<QOutputLine> outputLines; // Store all lines
    int visibleLines; // Number of lines visible in viewport
    static constexpr int CHUNK_SIZE = 10000; // Chunk size for loading
    bool isUpdatingDisplay; // 防止递归调用的标志
};

#endif // OUTPUTDISPLAYWIDGET_H