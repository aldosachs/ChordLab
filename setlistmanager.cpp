#include "setlistmanager.h"
#include <QIODevice>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QBrush>
#include <QStandardItem>
#include <QAbstractListModel>

/* SetlistManager::SetlistManager(QObject *parent) : QAbstractListModel(parent) {
    // Initialization code (if any)
}*/
SetlistManager::SetlistManager(QObject *parent) : QStandardItemModel(parent) {
    // Initialization code (if any)
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
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open setlist:" << filePath;
        return;
    }

    // 1. Create the Parent Item (The Setlist File)
    QFileInfo fileInfo(filePath);
    QStandardItem *parentItem = new QStandardItem(fileInfo.fileName()); // e.g., "Monday Songbook 1.set"
    parentItem->setEditable(false);
    // Make the parent look a bit different (e.g., bold)
    QFont parentFont = parentItem->font();
    parentFont.setBold(true);
    parentItem->setFont(parentFont);

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip header lines or empty lines
        if (line.startsWith("##") || line.isEmpty()) continue;

        // Clean up any quotes the user might have typed
        line.remove('"');

        // 2. Create the Child Item (The Song)
        // Extract just the song name for the UI by splitting at slashes
        QString displayTitle = line.split(QRegularExpression("[/\\\\]")).last();
        if (displayTitle.isEmpty()) displayTitle = line; // Fallback

        QStandardItem *childItem = new QStandardItem(displayTitle);
        childItem->setEditable(false);

        // 🤫 Secretly store the actual relative path inside the item!
        childItem->setData(line, FilePathRole);

        // 3. Append the Song underneath the Setlist
        parentItem->appendRow(childItem);
    }

    // 4. Append the fully loaded Setlist to the Model
    this->appendRow(parentItem);

    file.close();
}

/* void SetlistManager::loadSetFile(const QString &filePath) {
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
} */

void SetlistManager::setSetlists(const QStringList &files) {
    // 1. Notify the View that the model is about to change
    beginResetModel();

    m_items.clear();
    for (const QString &fileName : files) {
        SetItem item;
        item.title = fileName; // Use the filename as the display title
        item.filePath = ":/resources/setlists/" + fileName;
        m_items.append(item);
    }

    // 2. Notify the View that the change is finished
    endResetModel();
}

bool SetlistManager::moveRows(const QModelIndex &parent, int sourceRow, int count,
                              const QModelIndex &destinationParent, int destinationChild) {
    // 1. Tell Qt we are about to change the model layout
    beginMoveRows(parent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);

    // 2. Move the actual data in our m_items list
    for (int i = 0; i < count; ++i) {
        SetItem item = m_items.takeAt(sourceRow);
        m_items.insert(destinationChild, item);
    }

    // 3. Finalize the move
    endMoveRows();
    return true;
}

void SetlistManager::createBackup() {
    m_originalItems = m_items; // QList assignment is deep, so this is safe!
}

void SetlistManager::revertToOriginal() {
    beginResetModel(); // Tells the View to refresh entirely
    m_items = m_originalItems;
    endResetModel();
}

// Allow the list to accept movement (reordering)
Qt::DropActions SetlistManager::supportedDropActions() const {
    return Qt::MoveAction;
}

// Allow the items to be moved (dragged)
Qt::DropActions SetlistManager::supportedDragActions() const {
    return Qt::MoveAction;
}
