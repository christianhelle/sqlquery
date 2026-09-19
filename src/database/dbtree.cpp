#include "dbtree.h"

DbTree::DbTree(QTreeWidget *tree) :
    tree(tree) {
}

void DbTree::clear() {
    this->tree->clear();
}

QString getFileSize(const qint64 size) {
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    auto num = static_cast<double>(size);
    while (num >= 1024 && i.hasNext()) {
        unit = i.next();
        num /= 1024.0;
    }

    return QString().setNum(num, 'f', 2) + " " + unit;
}

void DbTree::populateTree(const DatabaseInfo &info) {
    this->clear();

    auto dbInfoNode = std::make_unique<QTreeWidgetItem>();
    dbInfoNode->setText(0, "Database Info");
    const auto dbInfoNodePtr = dbInfoNode.get();
    this->tree->addTopLevelItem(dbInfoNode.release());

    const auto addInfo = [dbInfoNodePtr](const QString &text) {
        auto node = std::make_unique<QTreeWidgetItem>();
        node->setText(0, text);
        dbInfoNodePtr->addChild(node.release());
    };

    if (info.provider == Provider::Sqlite) {
        addInfo(QString("File name: ").append(info.filename));
        addInfo(QString("Created on: ").append(info.creationDate.toLocalTime().toString()));
        addInfo(QString("File size: ").append(getFileSize(info.size)));
    } else {
        addInfo(QString("Provider: ").append(providerName(info.provider)));
        addInfo(QString("Server: ").append(info.server));
        addInfo(QString("Database: ").append(info.databaseName));
        addInfo(QString("Size: ").append(getFileSize(info.size)));
    }
    if (!info.databaseVersion.isEmpty())
        addInfo(QString("Version: ").append(info.databaseVersion.section('\n', 0, 0).trimmed()));

    auto tablesRootNode = std::make_unique<QTreeWidgetItem>();
    tablesRootNode->setText(0, "Tables");
    const auto tablesRootNodePtr = tablesRootNode.get();
    this->tree->addTopLevelItem(tablesRootNode.release());

    for (const auto &table : info.tables) {
        auto tableNode = std::make_unique<QTreeWidgetItem>(TableItemType);
        tableNode->setText(0, table.schema.isEmpty() ? table.name : table.schema + "." + table.name);
        tableNode->setData(0, SchemaRole, table.schema);
        tableNode->setData(0, TableNameRole, table.name);
        const auto tableNodePtr = tableNode.get();
        tablesRootNodePtr->addChild(tableNode.release());

        for (const auto &col : table.columns) {
            auto colName = std::make_unique<QTreeWidgetItem>();
            colName->setText(0, col.name);
            const auto colNamePtr = colName.get();
            tableNodePtr->addChild(colName.release());

            auto colOrdinal = std::make_unique<QTreeWidgetItem>();
            colOrdinal->setText(0, QString("Ordinal Position: ").append(QString::number(col.ordinal)));
            colNamePtr->addChild(colOrdinal.release());

            auto colDataType = std::make_unique<QTreeWidgetItem>();
            colDataType->setText(0, QString("Data Type: ").append(col.dataType));
            colNamePtr->addChild(colDataType.release());

            auto colNotNull = std::make_unique<QTreeWidgetItem>();
            colNotNull->setText(0, QString("Allow Null: ").append(!col.notNull ? "True" : "False"));
            colNamePtr->addChild(colNotNull.release());

            auto colPrimaryKey = std::make_unique<QTreeWidgetItem>();
            colPrimaryKey->setText(0, QString("Is Primary Key: ").append(col.primaryKey ? "True" : "False"));
            colNamePtr->addChild(colPrimaryKey.release());

            if (!col.defaultValue.isEmpty()) {
                auto defaultValue = std::make_unique<QTreeWidgetItem>();
                defaultValue->setText(0, QString("Default Value: ").append(col.defaultValue));
                colNamePtr->addChild(defaultValue.release());
            }
        }
    }

    this->tree->expandItem(dbInfoNodePtr);
    this->tree->expandItem(tablesRootNodePtr);
}
