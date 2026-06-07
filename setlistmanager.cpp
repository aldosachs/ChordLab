#include "setlistmanager.h"
#include <QIODevice>
#include <QFile>

SetlistManager::SetlistManager(QObject *parent) : QAbstractListModel(parent) {
    // Initialization code (if any)
}

void SetlistManager::loadFromSetFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    beginResetModel();
    m_items.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("\"")) { // Parse your format: "MSB 2\MSB 2.1 Yesterday [F 92 bpm]"
            SetItem item;
            item.filePath = line.mid(1, line.lastIndexOf('"') - 1); // Clean up the path
            item.title = item.filePath.section('\\', -1);           // Get the filename
            m_items.append(item);
        }
    }
    endResetModel();
}

bool SetlistManager::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    if (action == Qt::IgnoreAction) return true;

    // Logic to update your m_items vector:
    // 1. Capture the item currently at the drag source
    // 2. Remove it from the old position
    // 3. Insert it at the new 'row' position
    // 4. emit dataChanged(...) or beginMoveRows(...)

    return true;
}

QString SetlistManager::getFilePath(int row) const {
    // Safety check: make sure the row is valid
    if (row < 0 || row >= m_items.size()) {
        return QString();
    }
    return m_items[row].filePath;
}

// 1. The data() function tells the View what to show
QVariant SetlistManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    if (role == Qt::DisplayRole) {
        // Return the song title (or whatever you want displayed)
        return m_items[index.row()].title;
    }
    // You could also add Qt::DecorationRole here to show an icon if it's played!
    return QVariant();
}

// 2. The flags() function tells the View that the items are draggable
Qt::ItemFlags SetlistManager::flags(const QModelIndex &index) const {
    if (!index.isValid()) return Qt::ItemIsEnabled;

    // We return the default flags PLUS support for drag-and-drop
    return QAbstractListModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

// 3. The markAsPlayed logic
void SetlistManager::markAsPlayed(int row) {
    if (row >= 0 && row < m_items.size()) {
        m_items[row].isPlayed = true;

        // IMPORTANT: Tell the UI that the data has changed so it repaints
        emit dataChanged(index(row), index(row));
    }
}

void SetlistManager::loadSetFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open setlist:" << filePath;
        return;
    }

    m_items.clear(); // Clear existing
    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Skip header lines or empty lines
        if (line.startsWith("##") || line.isEmpty()) continue;

        // Assuming your .set format is: "Path\Title [Metadata]"
        SetItem item;
        item.title = line; // Adjust this parsing logic to match your .set format exactly
        item.filePath = line;

        m_items.append(item);
    }

    // Crucial: Tell the view to refresh
    beginResetModel();
    endResetModel();

    file.close();
}