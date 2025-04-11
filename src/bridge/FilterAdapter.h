#ifndef FILTERADAPTER_H
#define FILTERADAPTER_H

#include <QList>
#include <QString>
#include <QColor>
#include <QObject>
#include <memory>
#include "../ui/models/filterconfig.h"
#include "../core/FilterData.h"

/**
 * @brief Adapter class to bridge between Qt UI (FilterConfig) and core logic (Core::FilterData)
 * 
 * This class provides methods to convert between FilterConfig and Core::FilterData,
 * allowing the UI to work with Qt types while the core logic uses pure C++ types.
 */
class FilterAdapter : public QObject {
    Q_OBJECT
public:
    // Singleton instance
    static FilterAdapter& getInstance();    
    
    // Convert from Core::FilterData to FilterConfig
    FilterConfig toFilterConfig(const Core::FilterData& data);
    Core::FilterData toFilterData(const FilterConfig& config);

    // Helper methods for color conversion
    QString colorToString(const QColor& color) const;
    QColor stringToColor(const QString& colorStr) const;

signals:
    // Signals for filter events
    void filterCreated(const FilterConfig& filter);
    void filterRemoved(const QString& pattern);
    
private:
    // Private constructor for singleton
    FilterAdapter();
    
    // Prevent copying
    FilterAdapter(const FilterAdapter&) = delete;
    FilterAdapter& operator=(const FilterAdapter&) = delete;
};

#endif // FILTERADAPTER_H 