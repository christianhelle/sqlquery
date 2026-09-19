#ifndef SQLDIALECT_H
#define SQLDIALECT_H

#include <QString>

#include "connectioninfo.h"

class QSqlDatabase;

// What a Provider changes about SQL text: how an Identifier and a literal are
// delimited, how many rows a SELECT is capped at, and where the Schema is read
// from. Pure string logic, so every dialect can be checked without a server.
//
// The catalog queries return columns under fixed lower-case names, so the
// Analyzer reads every Provider the same way:
//   tablesQuery              -> table_schema, table_name
//   columnsQuery             -> ordinal, name, data_type, not_null,
//                               default_value, primary_key
//   serverInfoQuery          -> version, size_bytes
//   schemaFingerprintQuery   -> one value that changes when the Schema does
class SqlDialect {
public:
    virtual ~SqlDialect() = default;

    [[nodiscard]] virtual Provider provider() const = 0;

    // The Qt SQL driver behind this Provider.
    [[nodiscard]] virtual QString driverName() const = 0;

    // Points a connection made with driverName() at the Connection's target.
    virtual void configure(QSqlDatabase &database, const ConnectionInfo &info) const = 0;

    // The name of a Table or Column as written into SQL, delimited so a space,
    // a reserved word or the delimiter itself survives.
    [[nodiscard]] virtual QString quoteIdentifier(const QString &name) const = 0;

    // `schema.table`, each part delimited; just the table when schema is empty.
    [[nodiscard]] QString qualifiedName(const QString &schema, const QString &name) const;

    // A string literal.
    [[nodiscard]] virtual QString quoteLiteral(const QString &text) const;

    // Every row of a Table, capped at `limit` rows when limit > 0.
    [[nodiscard]] virtual QString selectAll(const QString &qualifiedTable, int limit = -1) const;

    [[nodiscard]] virtual QString tablesQuery() const = 0;

    [[nodiscard]] virtual QString columnsQuery(const QString &schema, const QString &table) const = 0;

    [[nodiscard]] virtual QString serverInfoQuery() const = 0;

    [[nodiscard]] virtual QString schemaFingerprintQuery() const = 0;

    // Reclaims unused space. Empty when the Provider has no such statement.
    [[nodiscard]] virtual QString shrinkStatement() const = 0;

    static const SqlDialect &forProvider(Provider provider);
};

class SqliteDialect final : public SqlDialect {
public:
    [[nodiscard]] Provider provider() const override { return Provider::Sqlite; }
    [[nodiscard]] QString driverName() const override { return QStringLiteral("QSQLITE"); }
    void configure(QSqlDatabase &database, const ConnectionInfo &info) const override;
    [[nodiscard]] QString quoteIdentifier(const QString &name) const override;
    [[nodiscard]] QString tablesQuery() const override;
    [[nodiscard]] QString columnsQuery(const QString &schema, const QString &table) const override;
    [[nodiscard]] QString serverInfoQuery() const override;
    [[nodiscard]] QString schemaFingerprintQuery() const override;
    [[nodiscard]] QString shrinkStatement() const override { return QStringLiteral("VACUUM"); }
};

class PostgresDialect final : public SqlDialect {
public:
    [[nodiscard]] Provider provider() const override { return Provider::PostgreSql; }
    [[nodiscard]] QString driverName() const override { return QStringLiteral("QPSQL"); }
    void configure(QSqlDatabase &database, const ConnectionInfo &info) const override;
    [[nodiscard]] QString quoteIdentifier(const QString &name) const override;
    [[nodiscard]] QString tablesQuery() const override;
    [[nodiscard]] QString columnsQuery(const QString &schema, const QString &table) const override;
    [[nodiscard]] QString serverInfoQuery() const override;
    [[nodiscard]] QString schemaFingerprintQuery() const override;
    [[nodiscard]] QString shrinkStatement() const override { return QStringLiteral("VACUUM"); }
};

class SqlServerDialect final : public SqlDialect {
public:
    [[nodiscard]] Provider provider() const override { return Provider::SqlServer; }
    [[nodiscard]] QString driverName() const override { return QStringLiteral("QODBC"); }
    void configure(QSqlDatabase &database, const ConnectionInfo &info) const override;
    [[nodiscard]] QString quoteIdentifier(const QString &name) const override;
    [[nodiscard]] QString quoteLiteral(const QString &text) const override;
    [[nodiscard]] QString selectAll(const QString &qualifiedTable, int limit = -1) const override;
    [[nodiscard]] QString tablesQuery() const override;
    [[nodiscard]] QString columnsQuery(const QString &schema, const QString &table) const override;
    [[nodiscard]] QString serverInfoQuery() const override;
    [[nodiscard]] QString schemaFingerprintQuery() const override;
    [[nodiscard]] QString shrinkStatement() const override { return QStringLiteral("DBCC SHRINKDATABASE(0)"); }

    // The ODBC connection string for a Connection.
    [[nodiscard]] static QString connectionString(const ConnectionInfo &info);
};

class MySqlDialect final : public SqlDialect {
public:
    [[nodiscard]] Provider provider() const override { return Provider::MySql; }
    [[nodiscard]] QString driverName() const override { return QStringLiteral("QMYSQL"); }
    void configure(QSqlDatabase &database, const ConnectionInfo &info) const override;
    [[nodiscard]] QString quoteIdentifier(const QString &name) const override;
    [[nodiscard]] QString quoteLiteral(const QString &text) const override;
    [[nodiscard]] QString tablesQuery() const override;
    [[nodiscard]] QString columnsQuery(const QString &schema, const QString &table) const override;
    [[nodiscard]] QString serverInfoQuery() const override;
    [[nodiscard]] QString schemaFingerprintQuery() const override;
    [[nodiscard]] QString shrinkStatement() const override { return {}; }
};

#endif // SQLDIALECT_H
