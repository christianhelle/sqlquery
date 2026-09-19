#include "sqldialect.h"

#include <QSqlDatabase>

#include "sqlidentifier.h"

namespace {
    // Wraps text in `open`/`close`, doubling every `close` inside it -- the
    // escape rule every Provider uses for its own delimiter.
    QString delimited(const QString &text, const QChar open, const QChar close) {
        QString escaped = text;
        escaped.replace(close, QString(2, close));
        return open + escaped + close;
    }

    void configureServer(QSqlDatabase &database, const ConnectionInfo &info) {
        database.setHostName(info.host);
        database.setPort(info.effectivePort());
        database.setDatabaseName(info.databaseName);
        database.setUserName(info.user);
        database.setPassword(info.password);
    }
}

const SqlDialect &SqlDialect::forProvider(const Provider provider) {
    static const SqliteDialect sqlite;
    static const PostgresDialect postgres;
    static const SqlServerDialect sqlServer;
    static const MySqlDialect mySql;
    switch (provider) {
        case Provider::PostgreSql:
            return postgres;
        case Provider::SqlServer:
            return sqlServer;
        case Provider::MySql:
            return mySql;
        case Provider::Sqlite:
            break;
    }
    return sqlite;
}

QString SqlDialect::qualifiedName(const QString &schema, const QString &name) const {
    if (schema.isEmpty())
        return quoteIdentifier(name);
    return quoteIdentifier(schema) + QLatin1Char('.') + quoteIdentifier(name);
}

QString SqlDialect::quoteLiteral(const QString &text) const {
    return delimited(text, QLatin1Char('\''), QLatin1Char('\''));
}

QStringList SqlDialect::splitScript(const QString &script) const {
    return script.split(QLatin1Char(';'), Qt::SkipEmptyParts);
}

QString SqlDialect::selectAll(const QString &qualifiedTable, const int limit) const {
    QString sql = QStringLiteral("SELECT * FROM ") + qualifiedTable;
    if (limit > 0)
        sql += QStringLiteral(" LIMIT ") + QString::number(limit);
    return sql;
}

// SQLite

void SqliteDialect::configure(QSqlDatabase &database, const ConnectionInfo &info) const {
    database.setDatabaseName(info.filePath);
}

QString SqliteDialect::quoteIdentifier(const QString &name) const {
    return quotedIdentifier(name);
}

QString SqliteDialect::tablesQuery() const {
    // Names starting sqlite_ are reserved for SQLite's own bookkeeping.
    return QStringLiteral(
        "SELECT '' AS table_schema, name AS table_name FROM sqlite_master "
        "WHERE type = 'table' AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\'");
}

QString SqliteDialect::columnsQuery(const QString &, const QString &table) const {
    return QStringLiteral(
               "SELECT cid AS ordinal, name, type AS data_type, \"notnull\" AS not_null, "
               "dflt_value AS default_value, pk AS primary_key FROM pragma_table_info(%1)")
            .arg(quoteLiteral(table));
}

QString SqliteDialect::serverInfoQuery() const {
    // The size is read off the file instead, which is what the user sees on disk.
    return QStringLiteral("SELECT sqlite_version() AS version, NULL AS size_bytes");
}

QString SqliteDialect::schemaFingerprintQuery() const {
    return QStringLiteral("PRAGMA schema_version");
}

// PostgreSQL

void PostgresDialect::configure(QSqlDatabase &database, const ConnectionInfo &info) const {
    configureServer(database, info);
}

QString PostgresDialect::quoteIdentifier(const QString &name) const {
    return quotedIdentifier(name);
}

QString PostgresDialect::tablesQuery() const {
    return QStringLiteral(
        "SELECT table_schema, table_name FROM information_schema.tables "
        "WHERE table_type = 'BASE TABLE' "
        "AND table_schema NOT IN ('pg_catalog', 'information_schema') "
        "ORDER BY table_schema, table_name");
}

QString PostgresDialect::columnsQuery(const QString &schema, const QString &table) const {
    // pg_catalog rather than information_schema: format_type keeps a declared
    // length, which information_schema.columns splits off into other columns.
    return QStringLiteral(
               "SELECT a.attnum AS ordinal, a.attname AS name, "
               "format_type(a.atttypid, a.atttypmod) AS data_type, "
               "a.attnotnull AS not_null, "
               "pg_get_expr(d.adbin, d.adrelid) AS default_value, "
               "EXISTS (SELECT 1 FROM pg_index i WHERE i.indrelid = a.attrelid "
               "AND i.indisprimary AND a.attnum = ANY (i.indkey)) AS primary_key "
               "FROM pg_attribute a "
               "LEFT JOIN pg_attrdef d ON d.adrelid = a.attrelid AND d.adnum = a.attnum "
               "WHERE a.attrelid = to_regclass(%1) AND a.attnum > 0 AND NOT a.attisdropped "
               "ORDER BY a.attnum")
            .arg(quoteLiteral(qualifiedName(schema, table)));
}

QString PostgresDialect::serverInfoQuery() const {
    return QStringLiteral(
        "SELECT version() AS version, pg_database_size(current_database()) AS size_bytes");
}

QString PostgresDialect::schemaFingerprintQuery() const {
    return QStringLiteral(
        "SELECT md5(COALESCE(string_agg(table_schema || '.' || table_name || '.' || column_name "
        "|| ':' || data_type, ',' ORDER BY table_schema, table_name, column_name), '')) "
        "FROM information_schema.columns "
        "WHERE table_schema NOT IN ('pg_catalog', 'information_schema')");
}

// SQL Server

void SqlServerDialect::configure(QSqlDatabase &database, const ConnectionInfo &info) const {
    // QODBC takes the whole connection string as its database name.
    database.setDatabaseName(connectionString(info));
}

QString SqlServerDialect::connectionString(const ConnectionInfo &info) {
    // A braced ODBC value may hold ; and =; a } inside it is doubled.
    const auto braced = [](const QString &value) {
        return delimited(value, QLatin1Char('{'), QLatin1Char('}'));
    };

    QStringList parts;
    parts << "DRIVER=" + braced(info.odbcDriver);
    parts << "SERVER=" + braced(info.host + "," + QString::number(info.effectivePort()));
    if (!info.databaseName.isEmpty())
        parts << "DATABASE=" + braced(info.databaseName);
    if (info.integratedAuth) {
        parts << "Trusted_Connection=yes";
    } else {
        parts << "UID=" + braced(info.user);
        parts << "PWD=" + braced(info.password);
    }
    if (info.trustServerCertificate)
        parts << "TrustServerCertificate=yes";
    return parts.join(QLatin1Char(';')) + QLatin1Char(';');
}

QString SqlServerDialect::quoteIdentifier(const QString &name) const {
    return delimited(name, QLatin1Char('['), QLatin1Char(']'));
}

QString SqlServerDialect::quoteLiteral(const QString &text) const {
    // N'' keeps characters outside the database's code page.
    return QLatin1Char('N') + SqlDialect::quoteLiteral(text);
}

QString SqlServerDialect::selectAll(const QString &qualifiedTable, const int limit) const {
    if (limit > 0)
        return QStringLiteral("SELECT TOP (%1) * FROM %2").arg(limit).arg(qualifiedTable);
    return QStringLiteral("SELECT * FROM ") + qualifiedTable;
}

QString SqlServerDialect::tablesQuery() const {
    return QStringLiteral(
        "SELECT s.name AS table_schema, t.name AS table_name FROM sys.tables t "
        "JOIN sys.schemas s ON s.schema_id = t.schema_id "
        "WHERE t.is_ms_shipped = 0 ORDER BY s.name, t.name");
}

QString SqlServerDialect::columnsQuery(const QString &schema, const QString &table) const {
    // sys.columns keeps lengths in bytes; the declared type is rebuilt from them
    // so it reads the way it was written, nvarchar(50) rather than nvarchar.
    return QStringLiteral(
               "SELECT c.column_id AS ordinal, c.name AS name, "
               "TYPE_NAME(c.user_type_id) + CASE "
               "WHEN TYPE_NAME(c.user_type_id) IN ('varchar', 'char', 'varbinary', 'binary') "
               "THEN '(' + CASE WHEN c.max_length = -1 THEN 'max' "
               "ELSE CAST(c.max_length AS varchar(10)) END + ')' "
               "WHEN TYPE_NAME(c.user_type_id) IN ('nvarchar', 'nchar') "
               "THEN '(' + CASE WHEN c.max_length = -1 THEN 'max' "
               "ELSE CAST(c.max_length / 2 AS varchar(10)) END + ')' "
               "WHEN TYPE_NAME(c.user_type_id) IN ('decimal', 'numeric') "
               "THEN '(' + CAST(c.precision AS varchar(10)) + ',' + CAST(c.scale AS varchar(10)) + ')' "
               "ELSE '' END AS data_type, "
               "CASE WHEN c.is_nullable = 0 THEN 1 ELSE 0 END AS not_null, "
               "OBJECT_DEFINITION(c.default_object_id) AS default_value, "
               "CASE WHEN EXISTS (SELECT 1 FROM sys.indexes i JOIN sys.index_columns ic "
               "ON ic.object_id = i.object_id AND ic.index_id = i.index_id "
               "WHERE i.object_id = c.object_id AND i.is_primary_key = 1 "
               "AND ic.column_id = c.column_id) THEN 1 ELSE 0 END AS primary_key "
               "FROM sys.columns c WHERE c.object_id = OBJECT_ID(%1) ORDER BY c.column_id")
            .arg(quoteLiteral(qualifiedName(schema, table)));
}

QString SqlServerDialect::serverInfoQuery() const {
    return QStringLiteral(
        "SELECT @@VERSION AS version, "
        "CAST(SUM(CAST(size AS bigint)) * 8192 AS bigint) AS size_bytes FROM sys.database_files");
}

QString SqlServerDialect::schemaFingerprintQuery() const {
    return QStringLiteral(
        "SELECT CONCAT(COUNT(*), ':', COALESCE(CHECKSUM_AGG(CHECKSUM(TABLE_SCHEMA, TABLE_NAME, "
        "COLUMN_NAME, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH)), 0)) FROM INFORMATION_SCHEMA.COLUMNS");
}

// MySQL

void MySqlDialect::configure(QSqlDatabase &database, const ConnectionInfo &info) const {
    configureServer(database, info);
}

QString MySqlDialect::quoteIdentifier(const QString &name) const {
    return delimited(name, QLatin1Char('`'), QLatin1Char('`'));
}

QString MySqlDialect::quoteLiteral(const QString &text) const {
    // Unless NO_BACKSLASH_ESCAPES is set, a backslash escapes inside a literal.
    QString escaped = text;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    return SqlDialect::quoteLiteral(escaped);
}

QString MySqlDialect::tablesQuery() const {
    // The connection's database is the Schema; MySQL has no namespace below it.
    return QStringLiteral(
        "SELECT '' AS table_schema, TABLE_NAME AS table_name FROM information_schema.TABLES "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_TYPE = 'BASE TABLE' ORDER BY TABLE_NAME");
}

QString MySqlDialect::columnsQuery(const QString &, const QString &table) const {
    return QStringLiteral(
               "SELECT ORDINAL_POSITION AS ordinal, COLUMN_NAME AS name, COLUMN_TYPE AS data_type, "
               "IS_NULLABLE = 'NO' AS not_null, COLUMN_DEFAULT AS default_value, "
               "COLUMN_KEY = 'PRI' AS primary_key FROM information_schema.COLUMNS "
               "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = %1 ORDER BY ORDINAL_POSITION")
            .arg(quoteLiteral(table));
}

QString MySqlDialect::serverInfoQuery() const {
    return QStringLiteral(
        "SELECT VERSION() AS version, (SELECT COALESCE(SUM(DATA_LENGTH + INDEX_LENGTH), 0) "
        "FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()) AS size_bytes");
}

QString MySqlDialect::schemaFingerprintQuery() const {
    return QStringLiteral(
        "SELECT CONCAT(COUNT(*), ':', COALESCE(SUM(CRC32(CONCAT_WS('.', TABLE_NAME, COLUMN_NAME, "
        "COLUMN_TYPE))), 0)) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE()");
}
