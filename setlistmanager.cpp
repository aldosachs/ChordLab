#include "setlistmanager.h"
#include <QIODevice>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QBrush>
#include <QStandardItem>
#include <QAbstractListModel>

SetlistManager::SetlistManager(QObject *parent) : QStandardItemModel(parent) {
    // Initialization code (if any)
}

bool SetlistManager::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    if (action == Qt::IgnoreAction) return true;

    // Logic to update:
    // 1. Capture the item currently at the drag source
    // 2. Remove it from the old position
    // 3. Insert it at the new 'row' position
    // 4. emit dataChanged(...) or beginMoveRows(...)

    return true;
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
Qt::ItemFlags SetlistManager::flags(const QModelIndex &index) const {
    if (!index.isValid()) return Qt::ItemIsEnabled;

    // We return the default flags PLUS support for drag-and-drop
    return QStandardItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

// 3. The markAsPlayed logic
void SetlistManager::markAsPlayed(const QModelIndex &index) {
    QStandardItem *item = this->itemFromIndex(index);
    if (item) {
        item->setForeground(QBrush(Qt::gray)); // Grey it out!
    }
}

void SetlistManager::loadSetFile(const QString &filePath) {
    // 1. CLEAR the model
    beginResetModel();
    this->clear(); // <--- this clears all QStandardItems from the model
    endResetModel();

    // Now re-populate
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
}

void SetlistManager::setSetlists(const QStringList &files) {
    beginResetModel();
    this->clear(); // Wipes the whole model tree

    for (const QString &fileName : files) {
        QStandardItem *item = new QStandardItem(fileName);
        item->setData(":/resources/setlists/" + fileName, FilePathRole);
        this->appendRow(item);
    }
    endResetModel();
}

bool SetlistManager::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                              const QModelIndex &destinationParent, int destinationChild) {
    // QStandardItemModel's built-in moveRow function does the heavy lifting
    return this->moveRow(sourceParent, sourceRow, destinationParent, destinationChild);
}

void SetlistManager::revertToOriginal() {
    //    beginResetModel(); // Tells the View to refresh entirely
    //    loadSetFile(currentFilePath); // add and use filepath variable to this function???
    //    endResetModel();
}

// Allow the list to accept movement (reordering)
Qt::DropActions SetlistManager::supportedDropActions() const {
    return Qt::MoveAction;
}

// Allow the items to be moved (dragged)
Qt::DropActions SetlistManager::supportedDragActions() const {
    return Qt::MoveAction;
}




