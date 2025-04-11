#ifndef OUTPUTDISPLAYWIDGET_H
#define OUTPUTDISPLAYWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QContextMenuEvent>

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
    
    // Modified appendText to support colored segments
    void appendText(const QString &text, const QColor &color = Qt::black);
    void appendTextWithColors(const QStringList &textSegments, const QList<QColor> &colors);
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
    bool event(QEvent *event) override;

private slots:
    void onSystemThemeChanged(const QPalette &palette);

private:
    QLabel *headerLabel;
    InfoAreaWidget *infoArea;
    QTextEdit *textEditLines;
    QtBridge& bridge;
    int64_t workspaceId;
    QStringList currentLineInfos;
    
    void setupTextEdit();  // Helper method for text edit configuration
    void updateInfoArea(); // Helper method to update the info area
    QString formatLinePrefix(int outputLineIndex, int fileIndex, int lineIndex) const;
    int getLineStartPosition(int lineIndex) const;
};

#endif // OUTPUTDISPLAYWIDGET_H