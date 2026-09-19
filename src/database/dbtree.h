#ifndef DBTREE_H
#define DBTREE_H

#include <QTreeWidget>

#include "databaseinfo.h"

class DbTree {
public:
    // A Table node's item type, and the roles holding its Schema and bare name.
    // The text shows `schema.table`, which is not something to build SQL from.
    static constexpr int TableItemType = QTreeWidgetItem::UserType + 1;
    static constexpr int SchemaRole = Qt::UserRole;
    static constexpr int TableNameRole = Qt::UserRole + 1;

    explicit DbTree(QTreeWidget *);

    void clear();

    void populateTree(const DatabaseInfo &info);

private:
    QTreeWidget *tree;
};

#endif // DBTREE_H
