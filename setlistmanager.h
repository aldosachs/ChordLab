#ifndef SETLISTMANAGER_H
#define SETLISTMANAGER_H

#include <QAbstractListModel>
#include <QStringList>

struct SetItem {
    QString title;
    QString filePath;
    bool isPlayed = false;
};

class SetlistManager : public QAbstractListModel {
    Q_OBJECT

public:
    explicit SetlistManager(QObject *parent = nullptr);

    // Standard Model Overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Drag and Drop overrides for internal reordering
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;

    // Item Management
    void removeRowCustom(int row); // Custom function to delete an item
    void revertToOriginal();       // Restores from m_originalItems
    void createBackup();

    void markAsPlayed(int row);
    void loadSetFile(const QString &fileName);
    QString getFilePath(int row) const;
    void setSetlists(const QStringList &files);

private:
    QList<SetItem> m_items;
    QList<SetItem> m_originalItems; // Backup for undo/revert
};

#endif // SETLISTMANAGER_H