#ifndef SETLISTMANAGER_H
#define SETLISTMANAGER_H

#include <QStringList>
#include <QStandardItemModel>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

class SetlistManager : public QStandardItemModel {
    Q_OBJECT

public:
    explicit SetlistManager(QObject *parent = nullptr);

    // Standard Model Overrides
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Define a custom role to hold the actual file path invisibly
    enum ItemRoles {
        FilePathRole = Qt::UserRole + 1
    };

    // Drag and Drop overrides for internal reordering
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;

    // Item Management
    void revertToOriginal();       // Restores from m_originalItems
    void createBackup();

    void markAsPlayed(const QModelIndex &index);
    void loadSetFile(const QString &fileName);
    QString getFilePath(const QModelIndex &index) const;

private:
    QString m_currentLoadedSetlist;

};

#endif // SETLISTMANAGER_H