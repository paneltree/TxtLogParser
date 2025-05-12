#include "StyleManager.h"
#include <QApplication>

StyleManager& StyleManager::instance()
{
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() : QObject(nullptr)
{
    // Connect to palette change signals
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        // For Qt 5.15 and above, directly connect to paletteChanged signal
        connect(qApp, &QApplication::paletteChanged, this, &StyleManager::refreshStyles);
    #else
        // For older Qt versions, use applicationStateChanged as a workaround
        // This isn't as accurate but can help detect theme changes in many cases
        connect(qApp, &QApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive) {
                // Check if palette changed when application becomes active again
                static QPalette lastPalette = QApplication::palette();
                QPalette currentPalette = QApplication::palette();
                
                // Compare key colors to detect theme changes
                if (lastPalette.color(QPalette::Window) != currentPalette.color(QPalette::Window) ||
                    lastPalette.color(QPalette::WindowText) != currentPalette.color(QPalette::WindowText) ||
                    lastPalette.color(QPalette::Base) != currentPalette.color(QPalette::Base) ||
                    lastPalette.color(QPalette::Text) != currentPalette.color(QPalette::Text)) {
                    
                    lastPalette = currentPalette;
                    refreshStyles();
                }
            }
        });
    #endif
    
    // Also monitor theme changes via QEvent::ThemeChange
    qApp->installEventFilter(this);
}

StyleManager::~StyleManager()
{
    // 析构函数，通常为空
}

QString StyleManager::getTabStyle() const
{
    QPalette palette = QApplication::palette();
    
    // 获取系统主题颜色
    QColor tabBg = palette.color(QPalette::Window).lighter(110);
    QColor tabBorder = palette.color(QPalette::Mid);
    QColor selectedTabBg = palette.color(QPalette::Highlight);
    QColor selectedTabText = palette.color(QPalette::HighlightedText);
    QColor normalTabText = palette.color(QPalette::Text);
    
    // 创建使用系统颜色的样式表
    QString style = QString(R"(
        QTabBar::tab {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            padding: 6px 12px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: %4;
            color: %5;
            border-bottom-color: %4;
        }
        QTabBar::tab:!selected {
            margin-top: 2px;
        }
    )")
    .arg(tabBg.name())
    .arg(normalTabText.name())
    .arg(tabBorder.name())
    .arg(selectedTabBg.name())
    .arg(selectedTabText.name());
    
    return style;
}

QString StyleManager::getButtonStyle() const
{
    QPalette palette = QApplication::palette();
    
    // 获取系统主题颜色
    QColor buttonBg = palette.color(QPalette::Button);
    QColor buttonText = palette.color(QPalette::ButtonText);
    QColor buttonBorder = palette.color(QPalette::Mid);
    QColor buttonHoverBg = buttonBg.lighter(110);
    QColor buttonPressedBg = buttonBg.darker(110);
    
    // 创建使用系统颜色的按钮样式表
    QString style = QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            padding: 5px 10px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %4;
        }
        QPushButton:pressed {
            background-color: %5;
        }
    )")
    .arg(buttonBg.name())
    .arg(buttonText.name())
    .arg(buttonBorder.name())
    .arg(buttonHoverBg.name())
    .arg(buttonPressedBg.name());
    
    return style;
}

void StyleManager::refreshStyles()
{
    // 发出样式变化信号，通知所有连接的对象
    emit stylesChanged();
}

bool StyleManager::eventFilter(QObject *obj, QEvent *event)
{
    // Check for theme change events
    if (event->type() == QEvent::ThemeChange) {
        // When the system theme changes, refresh all styles
        refreshStyles();
        return false; // Allow the event to propagate
    }
    
    // Allow the event to propagate to other filters
    return QObject::eventFilter(obj, event);
}