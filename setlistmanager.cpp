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