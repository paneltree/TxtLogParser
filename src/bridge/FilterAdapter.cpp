#include "FilterAdapter.h"
#include "../core/Logger.h"

FilterAdapter& FilterAdapter::getInstance() {
    static FilterAdapter instance;
    return instance;
}

FilterAdapter::FilterAdapter() {
}

FilterConfig FilterAdapter::toFilterConfig(const Core::FilterData& data) {
    Core::Logger::getInstance().debug("Converting FilterData to FilterConfig");
    
    FilterConfig config(
        data.getId(),
        data.getRow(),
        QString::fromStdString(data.getPattern()),
        data.isCaseSensitive(),
        data.isWholeWord(),
        data.isRegex(),
        data.isEnabled(),
        stringToColor(QString::fromStdString(data.getColor()))
    );
    
    Core::Logger::getInstance().debug("Created FilterConfig with pattern: " + 
                                    config.filterPattern.toStdString() + 
                                    ", color: " + config.color.name().toStdString());
    
    return config;
}

Core::FilterData FilterAdapter::toFilterData(const FilterConfig& config) {
    Core::Logger::getInstance().debug("Converting FilterConfig to FilterData");
    
    Core::FilterData data(
        config.filterId,
        config.filterRow,
        config.filterPattern.toStdString(),
        config.caseSensitive,
        config.wholeWord,
        config.isRegex,
        config.enabled,
        config.color.name().toStdString()
    );

    Core::Logger::getInstance().debug("Created FilterData with pattern: " + 
                                    data.getPattern() + 
                                    ", color: " + data.getColor());
    
    return data;
}


QString FilterAdapter::colorToString(const QColor& color) const {
    return color.name();
}

QColor FilterAdapter::stringToColor(const QString& colorStr) const {
    return QColor(colorStr);
} 