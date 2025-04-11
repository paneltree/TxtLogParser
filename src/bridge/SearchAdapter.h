#ifndef SEARCHADAPTER_H
#define SEARCHADAPTER_H

#include <QList>
#include <QString>
#include <QColor>
#include <QObject>
#include <memory>
#include "../ui/models/searchconfig.h"
#include "../core/SearchData.h"

/**
 * @brief Adapter class to bridge between Qt UI (SearchConfig) and core logic (Core::SearchData)
 * 
 * This class provides methods to convert between SearchConfig and Core::SearchData,
 * allowing the UI to work with Qt types while the core logic uses pure C++ types.
 */
class SearchAdapter : public QObject {
    Q_OBJECT
public:
    // Singleton instance
    static SearchAdapter& getInstance();    
    
    // Convert from Core::SearchData to SearchConfig
    SearchConfig toSearchConfig(const Core::SearchData& data);
    Core::SearchData toSearchData(const SearchConfig& config);

    // Helper methods for color conversion
    QString colorToString(const QColor& color) const;
    QColor stringToColor(const QString& colorStr) const;

signals:
    // Signals for search events
    void searchCreated(const SearchConfig& search);
    void searchRemoved(const QString& pattern);
    
private:
    // Private constructor for singleton
    SearchAdapter();
    
    // Prevent copying
    SearchAdapter(const SearchAdapter&) = delete;
    SearchAdapter& operator=(const SearchAdapter&) = delete;
};

#endif // SEARCHADAPTER_H 