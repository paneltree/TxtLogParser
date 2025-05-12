#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QPalette>

/**
 * @brief 样式管理器类，提供全局样式管理功能
 * 
 * StyleManager 负责为应用程序的不同 UI 组件提供动态样式，
 * 可以适应系统主题变化，确保应用程序在不同系统主题下的一致表现。
 * 使用单例模式确保全局只有一个样式管理器实例。
 */
class StyleManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取 StyleManager 的单例实例
     * @return StyleManager 的单例引用
     */
    static StyleManager& instance();
    
    /**
     * @brief 获取标签页样式
     * @return 标签页的样式表字符串
     */
    QString getTabStyle() const;
    
    /**
     * @brief 获取按钮样式
     * @return 按钮的样式表字符串
     */
    QString getButtonStyle() const;
    
    /**
     * @brief 刷新所有样式（当主题改变时调用）
     */
    void refreshStyles();
    
signals:
    /**
     * @brief 当样式发生变化时发出的信号
     */
    void stylesChanged();
    
private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    StyleManager();
    
    /**
     * @brief 私有析构函数
     */
    ~StyleManager();
    
    // 禁止拷贝和赋值
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
};

#endif // STYLEMANAGER_H