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
    
    QTextBlock block = textEditor->document()->firstBlock();
    int blockNumber = 0;
    
    // Use a slightly muted color for line numbers for subtle contrast
    painter.setPen(QApplication::palette().color(QPalette::Text).darker(120));
    
    // Calculate text margin to match document margin of textEditLines
    int documentMargin = 5; // Same as textEditLines->document()->documentMargin()
    
    // Adjust the drawing area to account for scrollbar height (16px)
    int scrollbarHeight = 16;
    int effectiveHeight = height() - (textEditor->horizontalScrollBar()->isVisible() ? scrollbarHeight : 0);

    // Get QTextEdit document layout
    QAbstractTextDocumentLayout* layout = textEditor->document()->documentLayout();
    
    // Calculate adjustment for top padding to match exactly with textEdit
    // The -2 offset is needed because:
    // 1. QTextEdit renders text with internal padding that isn't directly accessible
    // 2. The QTextEdit's line height calculation includes font metrics that differ slightly 
    //    from how our custom widget draws text
    // 3. QTextDocument positions text blocks with subtle spacing different from raw text drawing
    // This adjustment precisely compensates for these rendering differences
    int topAdjustment = -2; // Fine-tuning vertical alignment
    
    // Draw line info text for visible blocks
    while (block.isValid() && blockNumber < lineInfos.size()) {
        // Calculate the position of the text based on the text editor's layout
        QRectF blockRect = layout->blockBoundingRect(block);
        int top = (int)blockRect.top() + verticalOffset + topAdjustment;
        int height = (int)blockRect.height();
        
        // Only draw if the block is visible in the viewport (accounting for scrollbar)
        if (top + height >= 0 && top <= effectiveHeight && blockNumber < lineInfos.size()) {
            // Draw text precisely aligned with textEditLines
            QRectF drawRect(documentMargin, top, width() - documentMargin * 2, height);
            painter.drawText(drawRect, Qt::AlignRight | Qt::AlignVCenter, lineInfos.at(blockNumber));
        }
        
        block = block.next();
        ++blockNumber;
    }
    
    // Draw a subtle separator line
    painter.setPen(QPen(QApplication::palette().color(QPalette::Mid).lighter(150), 1));
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
    : QWidget(parent), bridge(bridge), workspaceId(workspaceId)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    headerLabel = new QLabel("Output", this);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #0078d4;");
    layout->addWidget(headerLabel);
    
    // Create a custom container that will handle horizontal scrolling for both components
    QWidget *containerWidget = new QWidget(this);
    containerWidget->setObjectName("outputContainer");
    containerWidget->setStyleSheet(R"(
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
    )");
    
    // Create outer container widget to maintain consistent width regardless of scrollbar visibility
    QWidget *outerWidget = new QWidget(containerWidget);
    QHBoxLayout *outerLayout = new QHBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    
    // Create a scrollable area to contain both widgets
    QScrollArea *scrollArea = new QScrollArea(outerWidget);
    scrollArea->setObjectName("unifiedScrollArea");
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Create an inner widget to hold our components
    QWidget *innerWidget = new QWidget(scrollArea);
    QHBoxLayout *innerLayout = new QHBoxLayout(innerWidget);
    innerLayout->setContentsMargins(0, 0, 0, 0);
    innerLayout->setSpacing(0);
    
    textEditLines = new QTextEdit(this);
    
    infoArea = new InfoAreaWidget(textEditLines);
    infoArea->setFixedWidth(130);
    
    setupTextEdit();  // Configure text edit properties
    
    // Add widgets to the inner container
    innerLayout->addWidget(infoArea);
    innerLayout->addWidget(textEditLines);
    
    // Set the inner widget as the scroll area's widget
    scrollArea->setWidget(innerWidget);
    
    // Add scrollArea to the outer layout
    outerLayout->addWidget(scrollArea);
    
    // Add the outer widget to the main container's layout
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(outerWidget);
    
    // Add the container to the main layout
    layout->addWidget(containerWidget);
    setLayout(layout);
    
    // Connect for scrolling synchronization
    connect(textEditLines->verticalScrollBar(), &QScrollBar::valueChanged, 
            [this]() { infoArea->update(); });
    
    // Connect for horizontal scrollbar visibility changes
    connect(textEditLines->horizontalScrollBar(), &QScrollBar::rangeChanged,
            [this]() { infoArea->update(); });
            
    // Connect for content changes
    connect(textEditLines->document(), &QTextDocument::contentsChange, 
            [this]() { updateInfoArea(); });
            
    // Instead of using the deprecated paletteChanged signal, we'll handle palette 
    // changes via the event system in our event() override method
    // (The InfoAreaWidget already handles this via its event() method)
}

OutputDisplayWidget::~OutputDisplayWidget()
{
}

bool OutputDisplayWidget::event(QEvent *event)
{
    // Handle application palette change events
    if (event->type() == QEvent::ApplicationPaletteChange) {
        onSystemThemeChanged(QApplication::palette());
        return true;
    }
    return QWidget::event(event);
}

void OutputDisplayWidget::onSystemThemeChanged(const QPalette &palette)
{
    // Update the info area when the system theme changes
    if (infoArea) {
        infoArea->update();
    }
}

void OutputDisplayWidget::setupTextEdit()
{
    textEditLines->setReadOnly(false);  // Needed for selection/highlighting
    textEditLines->setLineWrapMode(QTextEdit::NoWrap);
    
    // Use the same font for both textEditLines and infoArea
    QFont monoFont("Courier New", 10);
    textEditLines->setFont(monoFont);
    if (infoArea) {
        infoArea->setFont(monoFont);
    }
    
    textEditLines->setMinimumHeight(200);
    
    // Remove the border from the text edit for a seamless look
    textEditLines->setFrameShape(QFrame::NoFrame);
    
    // Ensure scroll bars are always visible when needed
    textEditLines->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEditLines->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Set consistent document margin for text alignment
    textEditLines->document()->setDocumentMargin(5);
    
    // Ensure the viewport has no margins that could affect alignment
    QWidget* viewPort = textEditLines->viewport();
    if (viewPort) {
        viewPort->setContentsMargins(0, 0, 0, 0);
    }
    
    // Set a stylesheet to ensure consistent text positioning
    textEditLines->setStyleSheet("QTextEdit { line-height: 100%; }");
    
    // Enable text selection and highlighting
    textEditLines->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
}

void OutputDisplayWidget::updateInfoArea()
{
    // This ensures the info area is updated when the text edit changes
    if (infoArea) {
        infoArea->update();
    }
}

void OutputDisplayWidget::appendText(const QString &text, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    
    QTextCursor cursor = textEditLines->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    textEditLines->setTextCursor(cursor);
    
    // Ensure the latest text is visible
    textEditLines->verticalScrollBar()->setValue(textEditLines->verticalScrollBar()->maximum());
    
    // Update the info area to stay in sync
    updateInfoArea();
}

void OutputDisplayWidget::appendTextWithColors(const QStringList &textSegments, const QList<QColor> &colors)
{
    if (textSegments.size() != colors.size()) {
        appendText("Error: Text segments and colors count mismatch");
        return;
    }    
    QTextCursor cursor = textEditLines->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    for (int i = 0; i < textSegments.size(); ++i) {
        QTextCharFormat format;
        format.setForeground(colors[i]);
        cursor.insertText(textSegments[i], format);
    }
    
    textEditLines->setTextCursor(cursor);
    textEditLines->verticalScrollBar()->setValue(textEditLines->verticalScrollBar()->maximum());
    
    // Update the info area to stay in sync
    updateInfoArea();
}

void OutputDisplayWidget::clearDisplay()
{
    textEditLines->clear();
    currentLineInfos.clear();
    infoArea->setLineInfos(currentLineInfos);
    updateInfoArea();
}

void OutputDisplayWidget::contextMenuEvent(QContextMenuEvent *event)
{
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
    
    QAction *clearAction = contextMenu.addAction("Clear");
    connect(clearAction, &QAction::triggered, this, &OutputDisplayWidget::clearDisplay);
    
    // Add copy action for selected text
    QAction *copyAction = contextMenu.addAction("Copy");
    connect(copyAction, &QAction::triggered, textEditLines, &QTextEdit::copy);
    copyAction->setEnabled(textEditLines->textCursor().hasSelection());
    
    contextMenu.exec(event->globalPos());
}

QString OutputDisplayWidget::formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const
{
    // Format: "[Fxxx:Lxxxx] " where xxx and xxxx are zero-padded numbers
    // Using 3 digits for file index and 4 digits for line number
    return QString("%1 [%2:%3]")
        .arg(outputLineIndex, 6, 10, QChar('0'))
        .arg(fileIndex, 3, 10, QChar('0'))
        .arg(lineIndex+1, 6, 10, QChar('0'));
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
    currentLineInfos.clear();
    
    //get the default text color from the system
    QColor defaultTextColor = QColor(qApp->palette().color(QPalette::Text));
    QList<QOutputLine> qOutputLines = bridge.getOutputStringList(workspaceId);
    int outputLineIndex = 0;
    
    // First collect all line infos
    for (const auto& qOutputLine : qOutputLines) {
        // Create line info for the InfoAreaWidget
        QString lineInfo = formatLinePrefix(outputLineIndex++, qOutputLine.m_fileRow, qOutputLine.m_lineIndex);
        currentLineInfos.append(lineInfo);
    }
    
    // Set the line infos to the InfoArea first
    infoArea->setLineInfos(currentLineInfos);
    
    // Then add the actual text content
    outputLineIndex = 0;
    for (const auto& qOutputLine : qOutputLines) {
        QStringList textSegments;
        QList<QColor> colors;

        // Use the already created line info
        QString lineInfo = currentLineInfos.at(outputLineIndex++);                
        for (const auto& qOutputSubLine : qOutputLine.m_subLines) {
            textSegments.append(qOutputSubLine.m_content);
            if (qOutputSubLine.m_color.isEmpty()) {
                //get the system's default color
                colors.append(defaultTextColor);
            } else {
                colors.append(QColor(qOutputSubLine.m_color));
            }
        }
        textSegments.append("\n");
        colors.append(Qt::black);
        appendTextWithColors(textSegments, colors);
    }
    
    textEditLines->verticalScrollBar()->setValue(textEditLines->verticalScrollBar()->maximum());
}

void OutputDisplayWidget::onNavigateToNextFilterMatch(int filterId)
{
    //get current cursor position
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextMatch: "
        "filterId: %1 lineIndex: %2 charIndex: %3 "
        ).arg(filterId).arg(lineIndex).arg(charIndex));
    
    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    //get the match positions
    bool ret = bridge.getNextMatchByFilter(workspaceId, filterId, lineIndex, charIndex
            , matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextMatch: "
        "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
        .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex)); 
    if(ret){
        //hight light the text ad line matchLineIndex , from matchCharStartIndex to matchCharEndIndex
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if(lineStartPosition != -1){
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousFilterMatch(int filterId)
{
    //get current cursor position
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousMatch: "
        "filterId: %1 lineIndex: %2 charIndex: %3 "
        ).arg(filterId).arg(lineIndex).arg(charIndex));
    
    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    //get the match positions
    bool ret = bridge.getPreviousMatchByFilter(workspaceId, filterId, lineIndex, charIndex
            , matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousMatch: "
        "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
        .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex)); 
    if(ret){
        //hight light the text ad line matchLineIndex , from matchCharEndIndex to  matchCharStartIndex
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if(lineStartPosition != -1){
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToNextSearchMatch(int searchId)
{
    //get current cursor position
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextSearchMatch: "
        "searchId: %1 lineIndex: %2 charIndex: %3 "
        ).arg(searchId).arg(lineIndex).arg(charIndex));
    
    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    //get the match positions
    bool ret = bridge.getNextMatchBySearch(workspaceId, searchId, lineIndex, charIndex
            , matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToNextSearchMatch: "
        "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
        .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex)); 
    if(ret){
        //hight light the text ad line matchLineIndex , from matchCharStartIndex to matchCharEndIndex
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if(lineStartPosition != -1){
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharStartIndex);
            cursor.setPosition(lineStartPosition + matchCharEndIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}

void OutputDisplayWidget::onNavigateToPreviousSearchMatch(int searchId)
{
    //get current cursor position
    QTextCursor cursor = textEditLines->textCursor();
    int lineIndex = cursor.blockNumber();
    int charIndex = cursor.positionInBlock();
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousSearchMatch: "
        "searchId: %1 lineIndex: %2 charIndex: %3 "
        ).arg(searchId).arg(lineIndex).arg(charIndex));
    
    int matchLineIndex = -1;
    int matchCharStartIndex = -1;
    int matchCharEndIndex = -1;
    //get the match positions
    bool ret = bridge.getPreviousMatchBySearch(workspaceId, searchId, lineIndex, charIndex
            , matchLineIndex, matchCharStartIndex, matchCharEndIndex);
    bridge.logInfo(QString("OutputDisplayWidget::onNavigateToPreviousSearchMatch: "
        "ret: %1 matchLineIndex: %2 matchCharStartIndex: %3 matchCharEndIndex: %4")
        .arg(ret).arg(matchLineIndex).arg(matchCharStartIndex).arg(matchCharEndIndex)); 
    if(ret){
        //hight light the text ad line matchLineIndex , from matchCharEndIndex to  matchCharStartIndex
        int lineStartPosition = getLineStartPosition(matchLineIndex);
        if(lineStartPosition != -1){
            QTextCursor cursor = textEditLines->textCursor();
            cursor.setPosition(lineStartPosition + matchCharEndIndex);
            cursor.setPosition(lineStartPosition + matchCharStartIndex, QTextCursor::KeepAnchor);
            textEditLines->setTextCursor(cursor);
        }
    }
}