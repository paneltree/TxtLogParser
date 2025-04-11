#include "SearchAdapter.h"
#include "../core/Logger.h"

SearchAdapter& SearchAdapter::getInstance() {
    static SearchAdapter instance;
    return instance;
}

SearchAdapter::SearchAdapter() {
}

SearchConfig SearchAdapter::toSearchConfig(const Core::SearchData& data) {
    Core::Logger::getInstance().debug("Converting SearchData to SearchConfig");
    
    SearchConfig config(
        data.getId(),
        data.getRow(),
        QString::fromStdString(data.getPattern()),
        data.isCaseSensitive(),
        data.isWholeWord(),
        data.isRegex(),
        data.isEnabled(),
        stringToColor(QString::fromStdString(data.getColor()))
    );
    
    Core::Logger::getInstance().debug("Created SearchConfig with pattern: " + 
                                    config.searchPattern.toStdString() + 
                                    ", color: " + config.color.name().toStdString());
    
    return config;
}

Core::SearchData SearchAdapter::toSearchData(const SearchConfig& config) {
    Core::Logger::getInstance().debug("Converting SearchConfig to SearchData");
    
    Core::SearchData data(
        config.searchId,
        config.searchRow,
        config.searchPattern.toStdString(),
        config.caseSensitive,
        config.wholeWord,
        config.isRegex,
        config.enabled,
        config.color.name().toStdString()
    );

    Core::Logger::getInstance().debug("Created SearchData with pattern: " + 
                                    data.getPattern() + 
                                    ", color: " + data.getColor());
    
    return data;
}


QString SearchAdapter::colorToString(const QColor& color) const {
    return color.name();
}

QColor SearchAdapter::stringToColor(const QString& colorStr) const {
    return QColor(colorStr);
} 