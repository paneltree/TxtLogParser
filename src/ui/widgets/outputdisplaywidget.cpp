#include "outputdisplaywidget.h"
#include <QVBoxLayout>
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
#include <QtCore/qpoint.h>
#include "bridge/QtBridge.h"

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

    // Get the viewport offset from the text editor's scrollbar
    int verticalOffset = -textEditor->verticalScrollBar()->value();

    // Use the same font as textEditLines to ensure matching text metrics
    QFont monoFont("Courier New", 10);
    painter.setFont(monoFont);



    //write to stdout a log
    static int logCount = 0;
    logCount++;
#if 0
    QTextStream(stdout) << "paneltree: InfoAreaWidget::paintEvent " << logCount 
                        << ", verticalOffset: " << verticalOffset
                        << ", blockCount: " << textEditor->document()->blockCount()
                        << ",lineInfos.size(): " << lineInfos.size()
                        << Qt::endl;
#endif
    // Use a slightly muted color for line numbers for subtle contrast
    painter.setPen(QApplication::palette().color(QPalette::Text).darker(120));
    
    // Calculate text margin to match document margin of textEditLines
    int documentMargin = 5; // Same as textEditLines->document()->documentMargin()
    
    // Adjust the drawing area to account for scrollbar height (16px)
    int scrollbarHeight = 16;
    int effectiveHeight = height() - (textEditor->horizontalScrollBar()->isVisible() ? scrollbarHeight : 0);

    // Get QTextEdit document layout
    QAbstractTextDocumentLayout* layout = textEditor->document()->documentLayout();
    int topAdjustment = -1; // Fine-tuning vertical alignment
    
    int blockNumber = 0;
    QTextBlock block = textEditor->document()->findBlockByNumber(blockNumber);

    if( textEditor->document()->blockCount() > 2){
        QRectF blockRect0 = layout->blockBoundingRect(block);
        block = block.next();
        QRectF blockRect1 = layout->blockBoundingRect(block);
        qreal oneBlockHeight = blockRect1.top() - blockRect0.top();
        int invisibleHeightUpper = -1 * (verticalOffset + topAdjustment);
        qint32 ignoredUpperBlockCount = invisibleHeightUpper / oneBlockHeight;
        blockNumber = ignoredUpperBlockCount;
        block = textEditor->document()->findBlockByNumber(blockNumber);
    }
    if(!block.isValid()){
        blockNumber = 0;
        block = textEditor->document()->findBlockByNumber(blockNumber);
    }
    //QTextStream(stdout) << "paneltree: InfoAreaWidget::paintEvent " << logCount 
    //    << ", begin from blockNumber: " << blockNumber
    //    << ", blockCount: " << textEditor->document()->blockCount()
    //    << ", lineInfos.size(): " << lineInfos.size()
    //    << ", block.isValid(): " << block.isValid()
    //    << ", width(): " << width() << ", height(): " << height()
    //    << Qt::endl;
    // Draw line info text for visible blocks
    while (block.isValid() && blockNumber < lineInfos.size()) {
        // Calculate the position of the text based on the text editor's layout
        QRectF blockRect = layout->blockBoundingRect(block);
        int top = (int)blockRect.top() + verticalOffset + topAdjustment;
        int height = (int)blockRect.height();

        // Only draw if the block is visible in the viewport (accounting for scrollbar)

        if (top + height >= 0 ){
            if(top <= effectiveHeight && blockNumber < lineInfos.size()) {
                // Draw text precisely aligned with textEditLines
                QRectF drawRect(documentMargin, top, width() - documentMargin * 2, height);
                painter.drawText(drawRect, Qt::AlignRight | Qt::AlignVCenter, lineInfos.at(blockNumber));
            }else{
                //QTextStream(stdout) << "paneltree: InfoAreaWidget::paintEvent " << logCount 
                //    << ", ignored from blockNumber: " << blockNumber
                //    << Qt::endl;
                break;
            }
        }

        block = block.next();
        ++blockNumber;
    }

    // Draw a subtle separator line
    painter.setPen(QPen(QApplication::palette().color(QPalette::Mid), 1));
    painter.drawLine(width() - 1, 0, width() - 1, effectiveHeight);

    // Draw a fake scrollbar area at the bottom if horizontal scrollbar is visible
    if (textEditor->horizontalScrollBar()->isVisible()) {
        QColor scrollAreaColor = QApplication::palette().color(QPalette::Base);
        painter.fillRect(0, effectiveHeight, width(), scrollbarHeight, scrollAreaColor);
        
        // Draw separator line for the scrollbar area
        painter.setPen(QPen(QApplication::palette().color(QPalette::Mid), 1));
        painter.drawLine(0, effectiveHeight, width(), effectiveHeight);
    }
}

void InfoAreaWidget::setLineInfos(const QStringList &newLineInfos)
{
    lineInfos = newLineInfos;
    update(); // Repaint the widget
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
    QWidget *containerWidget = new QWidget(this);
    containerWidget->setObjectName("outputContainer");
    containerWidget->setStyleSheet(containerStyleSheet);
    layout->addWidget(containerWidget);

    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Scroll area for the text edit
    QScrollArea *scrollArea = new QScrollArea(containerWidget);
    scrollArea->setObjectName("unifiedScrollArea");
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    containerLayout->addWidget(scrollArea);

    // Inner widget to hold the text edit and info area
    innerWidget = new QWidget(scrollArea);
    scrollArea->setWidget(innerWidget);

    QHBoxLayout *innerLayout = new QHBoxLayout(innerWidget);
    innerLayout->setContentsMargins(0, 0, 0, 0);
    innerLayout->setSpacing(0);

    textEditLines = new QTextEdit(innerWidget);
    infoArea = new InfoAreaWidget(textEditLines);
    infoArea->setFixedWidth(130);
    setupTextEdit();

    // Add widgets to the inner container
    innerLayout->addWidget(infoArea);
    innerLayout->addWidget(textEditLines);

    connect(textEditLines->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &OutputDisplayWidget::onScrollBarMoved);
    //connect(textEditLines->verticalScrollBar(), &QScrollBar::valueChanged, 
    //        [this]() { infoArea->update(); });
    connect(textEditLines->horizontalScrollBar(), &QScrollBar::rangeChanged,
            [this]() { infoArea->update(); });
    connect(textEditLines->document(), &QTextDocument::contentsChange, 
            [this]() { updateInfoArea(); });

    // Estimate visible lines
    QFontMetrics fm(textEditLines->font());
    visibleLines = textEditLines->height() / fm.lineSpacing() + 5;
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::OutputDisplayWidget "
                        << "visibleLines: " << visibleLines
                        << ", textEditLines->height(): " << textEditLines->height()
                        << ", fm.lineSpacing(): " << fm.lineSpacing()
                        << Qt::endl;
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
    textEditLines->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEditLines->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEditLines->document()->setDocumentMargin(5);
    textEditLines->viewport()->setContentsMargins(0, 0, 0, 0);
    textEditLines->setStyleSheet("QTextEdit { line-height: 100%; }");
    textEditLines->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
}

void OutputDisplayWidget::updateInfoArea()
{
    if (infoArea) {
        infoArea->update();
    }
}

void OutputDisplayWidget::clearDisplay()
{
    textEditLines->clear();
    outputLines.clear();
    currentLineInfos.clear();
    infoArea->setLineInfos(currentLineInfos);
    updateInfoArea();
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
    // Update visible lines on resize
    QFontMetrics fm(textEditLines->font());
    visibleLines = textEditLines->height() / fm.lineSpacing() + 5;
    updateDisplay(textEditLines->verticalScrollBar()->value(), visibleLines);
}

QString OutputDisplayWidget::formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const
{
    return QString("%1 [%2:%3]")
        .arg(outputLineIndex, 6, 10, QChar('0'))
        .arg(fileIndex, 3, 10, QChar('0'))
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
    textEditLines->clear();
    outputLines.clear();
    currentLineInfos.clear();

    outputLines = bridge.getOutputStringList(workspaceId);
    outputLines.reserve(1000000); // Pre-allocate for large datasets

    int outputLineIndex = 0;
    for (const auto& qOutputLine : outputLines) {
        QString lineInfo = formatLinePrefix(outputLineIndex++, qOutputLine.m_fileRow, qOutputLine.m_lineIndex);
        currentLineInfos.append(lineInfo);
        if (outputLineIndex % CHUNK_SIZE == 0) {
            QApplication::processEvents();
        }
    }

    infoArea->setLineInfos(currentLineInfos);
    textEditLines->verticalScrollBar()->setRange(0, std::max(0, static_cast<int>(outputLines.size()) - visibleLines));
    //verify the scroll bar range
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::doUpdate "
                        << "textEditLines->verticalScrollBar()->maximum(): " 
                        << textEditLines->verticalScrollBar()->maximum()
                        << Qt::endl;
    updateDisplay(0, visibleLines);
}

void OutputDisplayWidget::updateDisplay(int startLine, int lineCount)
{
    // 防止递归调用
    if (isUpdatingDisplay) {
        return;
    }

    //verify the scroll bar range
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "1 textEditLines->verticalScrollBar()->maximum(): " 
                        << textEditLines->verticalScrollBar()->maximum()
                        << Qt::endl;
    isUpdatingDisplay = true; // 设置标志，表示正在进行更新

    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "startLine: " << startLine
                        << ", lineCount: " << lineCount
                        << ", visibleLines: " << visibleLines
                        << ", outputLines.size(): " << outputLines.size()
                        << Qt::endl;

    // 禁用滚动条信号
    QScrollBar* vScrollBar = textEditLines->verticalScrollBar();
    vScrollBar->blockSignals(true);

    textEditLines->setUpdatesEnabled(false);
    textEditLines->clear();
    //verify the scroll bar range
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "2 textEditLines->verticalScrollBar()->maximum(): " 
                        << textEditLines->verticalScrollBar()->maximum()
                        << Qt::endl;

    startLine = std::max(0, std::min(startLine, static_cast<int>(outputLines.size() - 1)));
    int endLine = std::min(startLine + lineCount, static_cast<int>(outputLines.size()));

    QTextDocument* doc = textEditLines->document();
    QTextCursor cursor(doc);
    cursor.beginEditBlock();

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

    // 在重新启用更新前设置滚动条位置
    //verify the scroll bar range
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "3 textEditLines->verticalScrollBar()->maximum(): " 
                        << textEditLines->verticalScrollBar()->maximum()
                        << Qt::endl;
    vScrollBar->setValue(startLine);//verify the scroll bar range
    QTextStream(stdout) << "paneltree: OutputDisplayWidget::updateDisplay "
                        << "4 textEditLines->verticalScrollBar()->maximum(): " 
                        << textEditLines->verticalScrollBar()->maximum()
                        << Qt::endl;
    
    // 恢复滚动条信号
    vScrollBar->blockSignals(false);
    textEditLines->setUpdatesEnabled(true);

    applyHighlighting();
    infoArea->update();
    
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
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
            // Update display to ensure the match is visible
            textEditLines->verticalScrollBar()->setValue(matchLineIndex);
            updateDisplay(matchLineIndex, visibleLines);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousFilterMatch(int filterId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
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
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
            textEditLines->verticalScrollBar()->setValue(matchLineIndex);
            updateDisplay(matchLineIndex, visibleLines);
        }
    }
}

void OutputDisplayWidget::onNavigateToNextSearchMatch(int searchId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
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
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
            textEditLines->verticalScrollBar()->setValue(matchLineIndex);
            updateDisplay(matchLineIndex, visibleLines);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousSearchMatch(int searchId)
{
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
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
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if (lineStartPosition != -1) {
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
            textEditLines->verticalScrollBar()->setValue(matchLineIndex);
            updateDisplay(matchLineIndex, visibleLines);
        }
    }
}