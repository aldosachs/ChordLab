#include "setlistmanager.h"
#include <QIODevice>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QBrush>
#include <QStandardItem>

SetlistManager::SetlistManager(QObject *parent) : QStandardItemModel(parent) {
    // Initialization code (if any)
}

QString SetlistManager::getFilePath(const QModelIndex &index) const {
    // 1. Safety check
    if (!index.isValid()) {
        return QString();
    }

    // 2. Get the specific item from the tree
    QStandardItem *item = itemFromIndex(index);
    if (item) {
        // 3. Extract our secretly stored file path!
        return item->data(FilePathRole).toString();
    }

    return QString();
}

// 2. The flags() function tells the View that the items are draggable
/* Qt::ItemFlags SetlistManager::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        // Allow dropping onto the empty background (root)
        return Qt::ItemIsDropEnabled;
    }

    // Combine standard flags with Drag & Drop permissions
    return QStandardItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

// 3. The markAsPlayed logic
void SetlistManager::markAsPlayed(const QModelIndex &index) {
    QStandardItem *item = this->itemFromIndex(index);
    if (item) {
        item->setForeground(QBrush(Qt::gray)); // Grey it out!
    }
}
*/
Qt::ItemFlags SetlistManager::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        // Empty background area of the view frame
        return Qt::NoItemFlags;
    }

    // Every item should be enabled and selectable
    Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    // LEVEL 0: Top-level item (This is a Setlist container node)
    if (!index.parent().isValid()) {
        // Setlists must be drop-enabled so songs can be dropped inside them
        return defaultFlags | Qt::ItemIsDropEnabled;
    }
    // LEVEL 1: Child item (This is an individual Song node)
    else {
        // Songs can be dragged to reorder, but explicitly CANNOT accept drops onto themselves!
        // This single line kills the accidental sub-nesting/child creation bug.
        return defaultFlags | Qt::ItemIsDragEnabled;
    }
}
void SetlistManager::loadSetFile(const QString &filePath) {
    // Save this path so we know what to reload later!
    m_currentLoadedSetlist = filePath;

    // 1. Re-populate the model
    beginInsertRows(QModelIndex(), 0, 0); // Optional: if you want more granular signals

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open setlist:" << filePath;
        return;
    }

    QFileInfo fileInfo(filePath);
    QStandardItem *parentItem = new QStandardItem(fileInfo.fileName());
    parentItem->setEditable(false);

    // parse lines
    // Make the parent visually distinct (bold)
    QFont parentFont = parentItem->font();
    parentFont.setBold(true);
    parentItem->setFont(parentFont);

    // 2. Read the file and add Songs (Child Nodes)
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip headers/comments and empty lines
        if (line.startsWith("##") || line.isEmpty()) continue;

        // Clean up any quotes the user might have typed
        line.remove('"');

        // 3. Create the Song (Child) Node
        // Extract just the filename from the path for the UI (split at slashes)
        // e.g., "MSB 1\MSB 1.1 Yellow submarine" -> "MSB 1.1 Yellow submarine"
        QString displayTitle = line.split(QRegularExpression("[/\\\\]")).last();
        if (displayTitle.isEmpty()) displayTitle = line; // Fallback just in case

        QStandardItem *childItem = new QStandardItem(displayTitle);
        childItem->setEditable(false);

        // store the FULL relative path inside the item so ChordLab can open it
        childItem->setData(line, FilePathRole);
        qDebug() << "-->" << line;
        // 4. Attach the song underneath the setlist parent
        parentItem->appendRow(childItem);
    }

    this->appendRow(parentItem); // Add the parent to the cleared model
    file.close();

    endInsertRows();
//    endResetModel();
}

void SetlistManager::revertToOriginal() {
    if (!m_currentLoadedSetlist.isEmpty()) {
        loadSetFile(m_currentLoadedSetlist);
    }
}

// Allow the list to accept movement (reordering)
Qt::DropActions SetlistManager::supportedDropActions() const {
    return Qt::MoveAction;
}

// Allow the items to be moved (dragged)
Qt::DropActions SetlistManager::supportedDragActions() const {
    return Qt::MoveAction;
}
