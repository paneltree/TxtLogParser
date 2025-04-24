#include "outputdisplaywidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPalette>
#include <QEvent>
#include <QtCore/qdebug.h>
#include <QtCore/qlocale.h>
#include <QtCore/qpoint.h>
#include <cstdio>
#include "bridge/QtBridge.h"

// retrun a string representation of the QRectF
QString rectToString(const QRect &rect)
{
    return QString("QRectF(%1, %2, %3, %4)(%5 x %6)")
        .arg(rect.x()).arg(rect.y())
        .arg(rect.x() + rect.width()).arg(rect.y() + rect.height())
        .arg(rect.width()).arg(rect.height());
}
// retrun a string representation of the QRectF
QString rectfToString(const QRectF &rect)
{
    return QString("QRectF(%1, %2, %3, %4)(%5 x %6)")
        .arg(rect.x()).arg(rect.y())
        .arg(rect.x() + rect.width()).arg(rect.y() + rect.height())
        .arg(rect.width()).arg(rect.height());
}


// InfoAreaWidget implementation
InfoAreaWidget::InfoAreaWidget(QTextEdit *editor) : QWidget(editor), textEditor(editor)
{
    // Use the exact same font as the text editor for perfect alignment
    setFont(QFont("Courier New", 10));
    
    // Ensure the widget doesn't capture mouse events (so user can't select it)
    setAttribute(Qt::WA_TransparentForMouseEvents);
    
    // Enable automatic updates when the application palette changes
    setAttribute(Qt::WA_StyledBackground, true);
    
    // Set content margins to match textEditor
    setContentsMargins(0, 0, 0, 0);
}

QSize InfoAreaWidget::sizeHint() const
{
    // Calculate width based on the content
    int width = 150; // Default width
    return QSize(width, 0);
}

void InfoAreaWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    // Use exactly the same background color as the text edit
    QColor bgColor = QApplication::palette().color(QPalette::Base);
    painter.fillRect(event->rect(), bgColor);
    if(m_startLine >= m_endLine || m_startLine < 0 || m_endLine < 0){
        return;
    }
    if(m_startLine >= lineInfos.size() || m_endLine > lineInfos.size()){
        return;
    }    

    // Get the viewport offset from the text editor's scrollbar
    int verticalOffset = -textEditor->verticalScrollBar()->value();

    // Use the same font as textEditLines to ensure matching text metrics
    QFont monoFont("Courier New", 10);
    painter.setFont(monoFont);

    //write to stdout a log
    // Use a slightly muted color for line numbers for subtle contrast
    painter.setPen(QApplication::palette().color(QPalette::Text).darker(120));
    
    // Calculate text margin to match document margin of textEditLines
    int documentMargin = 5; // Same as textEditLines->document()->documentMargin()
    
    // Adjust the drawing area to account for scrollbar height (16px)

    // Get QTextEdit document layout
    QAbstractTextDocumentLayout* layout = textEditor->document()->documentLayout();
    int topAdjustment = -2; // Fine-tuning vertical alignment
    
    QTextBlock block = textEditor->document()->findBlockByNumber(0);
    int currentLine = m_startLine;
    while (block.isValid() && currentLine < m_endLine) {
        QRectF blockRect = layout->blockBoundingRect(block);
        int top = (int)blockRect.top() +  topAdjustment;
        int height = (int)blockRect.height();

        QRectF drawRect(documentMargin, top, width() - documentMargin * 2, height);
        painter.drawText(drawRect, Qt::AlignRight | Qt::AlignVCenter, lineInfos.at(currentLine));
        block = block.next();
        ++currentLine;
    }

    // Draw a subtle separator line
    int scrollbarHeight = 16;
    int effectiveHeight = height() - (textEditor->horizontalScrollBar()->isVisible() ? scrollbarHeight : 0);
    painter.setPen(QPen(QApplication::palette().color(QPalette::Mid), 1));
    painter.drawLine(width() - 1, 0, width() - 1, effectiveHeight);
}

void InfoAreaWidget::setLineInfos(const QVector<QString> &newLineInfos)
{
    lineInfos = newLineInfos;
}
void InfoAreaWidget::setLineRange(int startLine, int endLine)
{
    m_startLine = startLine;
    m_endLine = endLine;
}

bool InfoAreaWidget::event(QEvent *event)
{
    // Handle palette change events
    if (event->type() == QEvent::PaletteChange) {
        update(); // Repaint with new palette colors
        return true;
    }
    return QWidget::event(event);
}

// OutputDisplayWidget implementation
OutputDisplayWidget::OutputDisplayWidget(int64_t workspaceId, QtBridge& bridge, QWidget *parent)
    : QWidget(parent), bridge(bridge), workspaceId(workspaceId), isUpdatingDisplay(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    headerLabel = new QLabel("Output", this);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #0078d4;");
    layout->addWidget(headerLabel);

    // Define container widget stylesheet
    const QString containerStyleSheet = R"(
        QWidget#outputContainer { 
            border: 1px solid palette(mid); 
            border-radius: 3px; 
        }
        QScrollBar:horizontal {
            height: 16px;
            background: palette(base);
            border-top: 1px solid palette(mid);
        }
        QScrollBar:vertical {
            width: 16px;
            background: palette(base);
            border-left: 1px solid palette(mid);
        }
        QScrollBar::handle:horizontal, QScrollBar::handle:vertical {
            background: palette(mid);
            min-width: 20px;
            min-height: 20px;
            border-radius: 3px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            width: 0px;
            height: 0px;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )";

    // Container widget and layout
    containerWidget = new QWidget(this);
    containerWidget->setObjectName("outputContainer");
    containerWidget->setStyleSheet(containerStyleSheet);
    layout->addWidget(containerWidget);

    // 创建主容器布局 - 使用网格布局以便更精确地控制滚动条位置
    QGridLayout *containerLayout = new QGridLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // 创建文本区域和行号区域部分
    contentWidget = new QWidget(containerWidget);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    
    // 创建自定义滚动条
    customVerticalScrollBar = new QScrollBar(Qt::Vertical, containerWidget);
    customHorizontalScrollBar = new QScrollBar(Qt::Horizontal, containerWidget);
    
    // 创建文本编辑控件和行号区域
    textEditLines = new QTextEdit(contentWidget);
    infoArea = new InfoAreaWidget(textEditLines);
    infoArea->setFixedWidth(130);
    
    // 禁用文本编辑器的内置滚动条
    textEditLines->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEditLines->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 初始化文本编辑器
    setupTextEdit();
    
    // 安装事件过滤器来拦截可能导致滚动条显示的事件
    textEditLines->installEventFilter(this);
    
    // 添加控件到内容布局
    contentLayout->addWidget(infoArea);
    contentLayout->addWidget(textEditLines);
    
    // 将所有控件添加到网格布局
    containerLayout->addWidget(contentWidget, 0, 0);
    containerLayout->addWidget(customVerticalScrollBar, 0, 1);
    containerLayout->addWidget(customHorizontalScrollBar, 1, 0);
    
    // 设置滚动条的最小大小和策略
    customVerticalScrollBar->setFixedWidth(16);
    customHorizontalScrollBar->setFixedHeight(16);
    
    // 连接信号槽
    connect(customVerticalScrollBar, &QScrollBar::valueChanged, this, &OutputDisplayWidget::onScrollBarMoved);
    connect(customHorizontalScrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        // 水平滚动
        textEditLines->horizontalScrollBar()->setValue(value);
    });
    
#if 0
    connect(textEditLines->document(), &QTextDocument::contentsChange, [this]() {
        if(infoArea) {
            infoArea->update();
        }
        updateScrollBarRanges();
    });
#endif
    textEditLines->clear();
    QTextCursor cursor(textEditLines->document());
    cursor.insertBlock();
    QRectF blockRect = textEditLines->document()->documentLayout()->blockBoundingRect(cursor.block());
    m_oneLineHeight = blockRect.height();
    textEditLines->clear();

    visibleLines = textEditLines->height() / m_oneLineHeight;
#if 0
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::OutputDisplayWidget "
                        << "visibleLines: " << visibleLines
                        << ", textEditLines->height(): " << textEditLines->height()
                        << ", m_oneLineHeight: " << m_oneLineHeight
                        << Qt::endl;
#endif
}

OutputDisplayWidget::~OutputDisplayWidget()
{
}

bool OutputDisplayWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        onSystemThemeChanged(QApplication::palette());
        return true;
    }
    return QWidget::event(event);
}

void OutputDisplayWidget::onSystemThemeChanged(const QPalette &palette)
{
    if (infoArea) {
        infoArea->update();
    }
}

void OutputDisplayWidget::setupTextEdit()
{
    textEditLines->setReadOnly(false);
    textEditLines->setLineWrapMode(QTextEdit::NoWrap);
    QFont monoFont("Courier New", 10);
    textEditLines->setFont(monoFont);
    if (infoArea) {
        infoArea->setFont(monoFont);
    }
    textEditLines->setMinimumHeight(200);
    textEditLines->setFrameShape(QFrame::NoFrame);
    
    // 确保滚动条策略一致为AlwaysOff
    textEditLines->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEditLines->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    //textEditLines->document()->setDocumentMargin(5);
    textEditLines->viewport()->setContentsMargins(0, 0, 0, 0);
    textEditLines->viewport()->installEventFilter(this);
    
    // 使用CSS样式强制隐藏滚动条
    textEditLines->setStyleSheet("QTextEdit { line-height: 100%; }"
                              "QTextEdit::-webkit-scrollbar { width: 0px; height: 0px; }"
                              "QTextEdit QScrollBar { width: 0px; height: 0px; background: transparent; }");
    
    textEditLines->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
}

void OutputDisplayWidget::clearDisplay()
{
    textEditLines->clear();
    outputLines.clear();
    currentLineInfos.clear();
    infoArea->setLineInfos(currentLineInfos);
    infoArea->setLineRange(0, 0);
    infoArea->update();
}

void OutputDisplayWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);
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

    QAction *clearAction = contextMenu.addAction("Clear");
    connect(clearAction, &QAction::triggered, this, &OutputDisplayWidget::clearDisplay);

    QAction *copyAction = contextMenu.addAction("Copy");
    connect(copyAction, &QAction::triggered, textEditLines, &QTextEdit::copy);
    copyAction->setEnabled(textEditLines->textCursor().hasSelection());

    contextMenu.exec(event->globalPos());
}

void OutputDisplayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    visibleLines = textEditLines->height() / m_oneLineHeight;

#if 0
    //print geometry info of containerWidget, contentWidget, customVerticalScrollBar,customHorizontalScrollBar
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::resizeEvent "
                        << "containerWidget: " << rectToString(containerWidget->geometry())
                        << ", contentWidget: " << rectToString(contentWidget->geometry())
                        << ", customVerticalScrollBar: " << rectToString(customVerticalScrollBar->geometry())
                        << ", customHorizontalScrollBar: " << rectToString(customHorizontalScrollBar->geometry())
                        << Qt::endl;
    //print geometry info of textEditLines, infoArea
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::resizeEvent "
                        << "textEditLines: " << rectToString(textEditLines->geometry())
                        << ", infoArea: " << rectToString(infoArea->geometry())
                        << Qt::endl;

#endif
#if 0
    // textEditLines, infoArea, 
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::resizeEvent "
                        << ", workspaceId: " << workspaceId
                        << ", visibleLines: " << visibleLines
                        << ", textEditLines->height(): " << textEditLines->height()
                        << ", m_oneLineHeight: " << m_oneLineHeight
                        << Qt::endl;
#endif
    updateDisplay(textEditLines->verticalScrollBar()->value(), visibleLines);
}

QString OutputDisplayWidget::formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const
{
    return QString("%1 [%2:%3]")
        .arg(outputLineIndex, 6, 10, QChar('0'))
        .arg(fileIndex, 2, 10, QChar('0'))
        .arg(lineIndex + 1, 6, 10, QChar('0'));
}

int OutputDisplayWidget::getLineStartPosition(int lineIndex) const
{
    QTextDocument* doc = textEditLines->document();
    QTextBlock block = doc->findBlockByNumber(lineIndex);
    if (block.isValid()) {
        return block.position();
    }
    return -1;
}

void OutputDisplayWidget::doUpdate()
{
    // 禁用滚动条更新信号
    customVerticalScrollBar->blockSignals(true);
    customHorizontalScrollBar->blockSignals(true);
    
    textEditLines->clear();
    outputLines.clear();
    currentLineInfos.clear();

    outputLines = bridge.getOutputStringList(workspaceId);
    outputLines.reserve(1000000); // Pre-allocate for large datasets
    currentLineInfos.reserve(outputLines.size());

    int outputLineIndex = 0;
    for (const auto& qOutputLine : outputLines) {
        QString lineInfo = formatLinePrefix(outputLineIndex++, qOutputLine.m_fileRow, qOutputLine.m_lineIndex);
        currentLineInfos.append(lineInfo);
        if (outputLineIndex % CHUNK_SIZE == 0) {
            QApplication::processEvents();
        }
    }

    infoArea->setLineInfos(currentLineInfos);
    
    // 更新自定义滚动条范围
    updateScrollBarRanges();
    
    // 重置滚动条位置
    customVerticalScrollBar->setValue(0);
    customHorizontalScrollBar->setValue(0);
    
    // 恢复滚动条信号
    customVerticalScrollBar->blockSignals(false);
    customHorizontalScrollBar->blockSignals(false);
    
    // 显示内容
    updateDisplay(0, visibleLines);
}

void OutputDisplayWidget::updateDisplay(int startLine, int lineCount)
{
    // 防止递归调用
    if (isUpdatingDisplay) {
        return;
    }

    isUpdatingDisplay = true; // 设置标志，表示正在进行更新
    
    // 动态计算当前可见行数，确保使用最新的高度值
    int currentHeight = textEditLines->height();
    if (currentHeight > 0 && m_oneLineHeight > 0) {
        visibleLines = currentHeight / m_oneLineHeight;
        lineCount = visibleLines; // 使用最新计算的可见行数
    }
    
#if 0
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "startLine: " << startLine
                        << ", lineCount: " << lineCount
                        << ", visibleLines: " << visibleLines
                        << ", outputLines.size(): " << outputLines.size()
                        << Qt::endl;
#endif
    // 禁用更新和信号
    customVerticalScrollBar->blockSignals(true);
    customHorizontalScrollBar->blockSignals(true);
    textEditLines->setUpdatesEnabled(false);
    textEditLines->clear();

    // 确保startLine在有效范围内
    startLine = std::max(0, std::min(startLine, static_cast<int>(outputLines.size() - 1)));
    int endLine = std::min(startLine + lineCount, static_cast<int>(outputLines.size()));

    QTextDocument* doc = textEditLines->document();
    QTextCursor cursor(doc);
    cursor.beginEditBlock();

    m_textEditLinesStartLine = startLine;
    m_textEditLinesEndLine = endLine;
    QColor defaultTextColor = QApplication::palette().color(QPalette::Text);
    for (int i = startLine; i < endLine; ++i) {
        const auto& qOutputLine = outputLines[i];
        for (const auto& qOutputSubLine : qOutputLine.m_subLines) {
            QTextCharFormat format;
            format.setForeground(qOutputSubLine.m_color.isEmpty() ? 
                                defaultTextColor : QColor(qOutputSubLine.m_color));
            cursor.insertText(qOutputSubLine.m_content, format);
        }
        cursor.insertBlock();
    }
    cursor.endEditBlock();
    //print rect of first block and last block
    QTextBlock firstBlock = doc->firstBlock();
    QTextBlock lastBlock = doc->lastBlock();
#if 0
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "firstBlock: " << firstBlock.text()
                        << ", lastBlock: " << lastBlock.text()
                        << ", firstBlockRect: " << rectfToString(doc->documentLayout()->blockBoundingRect(firstBlock))
                        << ", lastBlockRect: " << rectfToString(doc->documentLayout()->blockBoundingRect(lastBlock))
                        << Qt::endl;
#endif
    // 使自定义滚动条与内容同步
    customVerticalScrollBar->setValue(startLine);
    
    // 恢复信号和更新
    customVerticalScrollBar->blockSignals(false);
    customHorizontalScrollBar->blockSignals(false);
    textEditLines->setUpdatesEnabled(true);

    // 应用高亮并更新信息区域
    applyHighlighting();
    infoArea->setLineRange(m_textEditLinesStartLine, m_textEditLinesEndLine);
    infoArea->update();
    
    // 更新水平滚动条范围和位置
    updateScrollBarRanges();
    
    isUpdatingDisplay = false; // 更新完成，重置标志
}

void OutputDisplayWidget::applyHighlighting()
{
    // Example: Highlight "example" (replace with actual logic from bridge if needed)
    std::string highlightTerm = "example";
    if (highlightTerm.empty()) return;

    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(Qt::yellow);

    QTextCursor cursor(textEditLines->document());
    cursor.movePosition(QTextCursor::Start);

    int startLine = textEditLines->verticalScrollBar()->value();
    int endLine = std::min(startLine + visibleLines, static_cast<int>(outputLines.size()));

    for (int i = startLine; i < endLine; ++i) {
        QString lineText;
        for (const auto& subLine : outputLines[i].m_subLines) {
            lineText += subLine.m_content;
        }
        std::string line = lineText.toStdString();
        size_t pos = 0;
        while ((pos = line.find(highlightTerm, pos)) != std::string::npos) {
            int lineStart = cursor.position();
            cursor.setPosition(lineStart + pos);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, highlightTerm.length());
            cursor.mergeCharFormat(highlightFormat);
            pos += highlightTerm.length();
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }
}

void OutputDisplayWidget::onScrollBarMoved(int value)
{
    // 如果正在更新显示，不要响应滚动条事件
    if (isUpdatingDisplay) {
        return;
    }
    updateDisplay(value, visibleLines);
}

void OutputDisplayWidget::onNavigateToNextFilterMatch(int filterId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    lineIndex += m_textEditLinesStartLine;
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "filterId: %1 lineIndex: %2 charIndex: %3 ")
                           .arg(filterId).arg(lineIndex).arg(charIndex));

    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    bool ret = bridge.getNextMatchByFilter(workspaceId, filterId, lineIndex, charIndex,
                                           matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
                           .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex));
    if (ret) {
        if(matchLineIndex < 0 || matchLineIndex >= outputLines.size()){
            return;
        }
        if(matchLineIndex >= m_textEditLinesEndLine){
            int newStartLine = matchLineIndex - visibleLines + 1;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        else if(matchLineIndex < m_textEditLinesStartLine){
            int newStartLine = matchLineIndex;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        if(matchLineIndex < m_textEditLinesStartLine || matchLineIndex >= m_textEditLinesEndLine){
            bridge.logError(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "matchLineIndex: %1 is out of range [%2, %3)")
                           .arg(matchLineIndex).arg(m_textEditLinesStartLine).arg(m_textEditLinesEndLine));
            return;
        }
        int relativeLineIndex = matchLineIndex - m_textEditLinesStartLine;
        
        
        int lineStartPosition = getLineStartPosition(relativeLineIndex);
        if (lineStartPosition != -1) {
            
            // 使用自定义滚动条而不是内置滚动条

            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousFilterMatch(int filterId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    lineIndex += m_textEditLinesStartLine;
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousMatch: "
                           "filterId: %1 lineIndex: %2 charIndex: %3 ")
                           .arg(filterId).arg(lineIndex).arg(charIndex));

    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    bool ret = bridge.getPreviousMatchByFilter(workspaceId, filterId, lineIndex, charIndex,
                                               matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousMatch: "
                           "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
                           .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex));
    if (ret) {
        if(matchLineIndex < 0 || matchLineIndex >= outputLines.size()){
            return;
        }
        
        if(matchLineIndex >= m_textEditLinesEndLine){
            int newStartLine = matchLineIndex - visibleLines + 1;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        else if(matchLineIndex < m_textEditLinesStartLine){
            int newStartLine = matchLineIndex;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        if(matchLineIndex < m_textEditLinesStartLine || matchLineIndex >= m_textEditLinesEndLine){
            bridge.logError(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "matchLineIndex: %1 is out of range [%2, %3)")
                           .arg(matchLineIndex).arg(m_textEditLinesStartLine).arg(m_textEditLinesEndLine));
            return;
        }
        int relativeLineIndex = matchLineIndex - m_textEditLinesStartLine;
        
        
        int lineStartPosition = getLineStartPosition(relativeLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToNextSearchMatch(int searchId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    lineIndex += m_textEditLinesStartLine;
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextSearchMatch: "
                           "searchId: %1 lineIndex: %2 charIndex: %3 ")
                           .arg(searchId).arg(lineIndex).arg(charIndex));

    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    bool ret = bridge.getNextMatchBySearch(workspaceId, searchId, lineIndex, charIndex,
                                           matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextSearchMatch: "
                           "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
                           .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex));
    if (ret) {
        if(matchLineIndex < 0 || matchLineIndex >= outputLines.size()){
            return;
        }
        if(matchLineIndex >= m_textEditLinesEndLine){
            int newStartLine = matchLineIndex - visibleLines + 1;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        else if(matchLineIndex < m_textEditLinesStartLine){
            int newStartLine = matchLineIndex;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        if(matchLineIndex < m_textEditLinesStartLine || matchLineIndex >= m_textEditLinesEndLine){
            bridge.logError(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "matchLineIndex: %1 is out of range [%2, %3)")
                           .arg(matchLineIndex).arg(m_textEditLinesStartLine).arg(m_textEditLinesEndLine));
            return;
        }
        int relativeLineIndex = matchLineIndex - m_textEditLinesStartLine;
        
        
        int lineStartPosition = getLineStartPosition(relativeLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousSearchMatch(int searchId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    lineIndex += m_textEditLinesStartLine;
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousSearchMatch: "
                           "searchId: %1 lineIndex: %2 charIndex: %3 ")
                           .arg(searchId).arg(lineIndex).arg(charIndex));

    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    bool ret = bridge.getPreviousMatchBySearch(workspaceId, searchId, lineIndex, charIndex,
                                               matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousSearchMatch: "
                           "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
                           .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex));
    if (ret) {
        if(matchLineIndex < 0 || matchLineIndex >= outputLines.size()){
            return;
        }
        
        if(matchLineIndex >= m_textEditLinesEndLine){
            int newStartLine = matchLineIndex - visibleLines + 1;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        else if(matchLineIndex < m_textEditLinesStartLine){
            int newStartLine = matchLineIndex;
            if(newStartLine < 0){
                newStartLine = 0;
            }
            customVerticalScrollBar->setValue(newStartLine);
            //updateDisplay(newStartLine, visibleLines);
        }
        if(matchLineIndex < m_textEditLinesStartLine || matchLineIndex >= m_textEditLinesEndLine){
            bridge.logError(QString("OutputDisplayWidget::onNavigateToNextMatch: "
                           "matchLineIndex: %1 is out of range [%2, %3)")
                           .arg(matchLineIndex).arg(m_textEditLinesStartLine).arg(m_textEditLinesEndLine));
            return;
        }
        int relativeLineIndex = matchLineIndex - m_textEditLinesStartLine;
        
        
        int lineStartPosition = getLineStartPosition(relativeLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::updateScrollBarRanges()
{
    int maxValue = std::max(0, static_cast<int>(outputLines.size() - visibleLines));
    customVerticalScrollBar->setRange(0, maxValue);
    customVerticalScrollBar->setPageStep(visibleLines);
    customVerticalScrollBar->setSingleStep(1);
    
    QScrollBar* textHorizontalScrollBar = textEditLines->horizontalScrollBar();
    customHorizontalScrollBar->setRange(0, textHorizontalScrollBar->maximum());
    customHorizontalScrollBar->setPageStep(textHorizontalScrollBar->pageStep());
    customHorizontalScrollBar->setSingleStep(textHorizontalScrollBar->singleStep());
    customHorizontalScrollBar->setValue(textHorizontalScrollBar->value());
#if 0
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateScrollBarRanges "
                        << "vertical range: 0-" << maxValue
                        << ", horizontal range: 0-" << textHorizontalScrollBar->maximum()
                        << ", outputLines.size(): " << outputLines.size()
                        << ", visibleLines: " << visibleLines
                        << ", value: " << customVerticalScrollBar->value()
                        << Qt::endl;
#endif
}

bool OutputDisplayWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 监听textEditLines的事件
    if (watched == textEditLines|| watched == textEditLines->viewport()) {
        // 处理滚轮事件，转发给自定义滚动条
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            // 将滚轮事件转发给自定义垂直滚动条
            if (wheelEvent->angleDelta().y() != 0) { // 垂直滚动
                int delta = wheelEvent->angleDelta().y() / 120; // 每120代表一个滚动单位
                int newValue = customVerticalScrollBar->value() - delta * 3; // 3行/次
                customVerticalScrollBar->setValue(newValue);
            }
            else if (wheelEvent->angleDelta().x() != 0) { // 水平滚动
                int delta = wheelEvent->angleDelta().x() / 120;
                int newValue = customHorizontalScrollBar->value() - delta * 20; // 20像素/次
                customHorizontalScrollBar->setValue(newValue);
            }
            return true; // 阻止事件被textEditLines处理
        }
        // 拦截可能导致滚动条显示的其他事件
        else if (event->type() == QEvent::ScrollPrepare 
                 || event->type() == QEvent::Scroll
                ){
            return true; // 阻止事件被textEditLines处理
        }
    }
    
    // 允许其他事件正常处理
    return QWidget::eventFilter(watched, event);
}