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

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_items.size(); }
//    int rowCount(const QModelIndex &parent);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Drag and Drop support
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void markAsPlayed(int row);
    void loadFromSetFile(const QString &fileName);
    QString getFilePath(int row) const;
    void loadSetFile(const QString &filePath);
    void setSetlists(const QStringList &files);


private:
    QList<SetItem> m_items;
};

#endif // SETLISTMANAGER_H