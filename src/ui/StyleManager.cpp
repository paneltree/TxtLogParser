#include "StyleManager.h"
#include <QApplication>

StyleManager& StyleManager::instance()
{
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() : QObject(nullptr)
{
    // 在较新的 Qt 版本中，可以连接 QApplication 的 paletteChanged 信号
    // 在 Qt 5.15 以上版本可使用：
    // connect(qApp, &QApplication::paletteChanged, this, &StyleManager::refreshStyles);
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