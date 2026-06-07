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