#include "dbanalyzer.h"

#include <QFileInfo>

#include "sqldialect.h"

namespace {
    // Catalog columns are aliased in lower case, but a driver may still hand
    // them back upper-cased.
    int columnIndex(const QueryResult &result, const QString &name) {
        for (int i = 0; i < result.columns.size(); ++i) {
            if (result.columns.at(i).compare(name, Qt::CaseInsensitive) == 0)
                return i;
        }
        return -1;
    }

    QVariant valueAt(const QueryRow &row, const int index) {
        return index >= 0 && index < row.values.size() ? row.values.at(index) : QVariant();
    }
}

DbAnalyzer::DbAnalyzer(IDatabase *database)
    : database(database) {
}

bool DbAnalyzer::analyze(DatabaseInfo &info) const {
    const ConnectionInfo connection = this->database->connection();
    info.provider = connection.provider;

    if (connection.isFile()) {
        const QFileInfo file(connection.filePath);
        info.filename = file.fileName();
        info.size = file.size();
        info.creationDate = file.birthTime();
    } else {
        info.filename = connection.displayName();
        info.server = connection.host + ":" + QString::number(connection.effectivePort());
        info.databaseName = connection.databaseName;
    }

    if (!loadTables(info))
        return false;

    loadColumns(info);
    loadServerInfo(info);

    return true;
}

bool DbAnalyzer::loadTables(DatabaseInfo &info) const {
    const QueryResult result = this->database->runStatement(this->database->dialect().tablesQuery());
    if (!result.ok) {
        return false;
    }

    const int schemaIdx = columnIndex(result, "table_schema");
    const int nameIdx = columnIndex(result, "table_name");
    for (const auto &row: result.rows) {
        Table table;
        table.schema = valueAt(row, schemaIdx).toString();
        table.name = valueAt(row, nameIdx).toString();
        info.tables.append(table);
    }

    return true;
}

void DbAnalyzer::loadColumns(DatabaseInfo &info) const {
    const SqlDialect &dialect = this->database->dialect();
    for (auto &table: info.tables) {
        const QueryResult result = this->database->runStatement(dialect.columnsQuery(table.schema, table.name));
        if (!result.ok) {
            continue;
        }

        const int ordinalIdx = columnIndex(result, "ordinal");
        const int nameIdx = columnIndex(result, "name");
        const int typeIdx = columnIndex(result, "data_type");
        const int notNullIdx = columnIndex(result, "not_null");
        const int defaultIdx = columnIndex(result, "default_value");
        const int pkIdx = columnIndex(result, "primary_key");

        for (const auto &row: result.rows) {
            Column col;
            col.ordinal = valueAt(row, ordinalIdx).toInt();
            col.name = valueAt(row, nameIdx).toString();
            col.dataType = valueAt(row, typeIdx).toString();
            col.notNull = valueAt(row, notNullIdx).toBool();
            col.defaultValue = valueAt(row, defaultIdx).toString();
            col.primaryKey = valueAt(row, pkIdx).toBool();
            table.columns.append(col);
        }
    }
}

void DbAnalyzer::loadServerInfo(DatabaseInfo &info) const {
    const QueryResult result = this->database->runStatement(this->database->dialect().serverInfoQuery());
    if (!result.ok || result.rows.isEmpty())
        return;

    const QueryRow &row = result.rows.first();
    info.databaseVersion = valueAt(row, columnIndex(result, "version")).toString();

    // A file's size is what the user sees on disk, so it is kept over the server's figure.
    const QVariant size = valueAt(row, columnIndex(result, "size_bytes"));
    if (info.provider != Provider::Sqlite && !size.isNull())
        info.size = size.toLongLong();
}
