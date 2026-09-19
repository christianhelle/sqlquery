#ifndef QUERYEXECUTOR_H
#define QUERYEXECUTOR_H

#include <QList>
#include <QString>
#include <QStringList>

#include "idatabase.h"
#include "queryresult.h"

class QAbstractItemModel;

class QueryExecutor {
public:
    explicit QueryExecutor(IDatabase *database);

    QList<QueryResult> runScript(const QString &script, QStringList *errors = nullptr);

    QList<QueryResult> runStatements(const QStringList &statements,
                                     QStringList *errors = nullptr);

    // Every row of a Table, capped at `limit` rows when limit > 0. `schema` is
    // the Table's namespace, empty for Providers without one.
    [[nodiscard]] QueryResult previewTable(const QString &tableName, int limit = -1,
                                           const QString &schema = {}) const;

    // Drops a Table. The name is delimited here rather than by the caller, so
    // a Table whose name holds a quote or a reserved word drops like any other
    // and no caller outside this module has to build the statement.
    QueryResult dropTable(const QString &tableName, const QString &schema = {}) const;

    // Display variants. Each returns a lazily fetched model whose rows are read
    // in pages as they are scrolled into view, so a result set of any size can
    // be browsed. The caller owns the returned models.
    QList<QAbstractItemModel *> runStatementsPaged(const QStringList &statements,
                                                   QStringList *errors = nullptr) const;

    // Splits a Script into its statements and runs them. Splitting lives in
    // the database module rather than in a caller, and the Dialect decides
    // where one piece ends: semicolons for most Providers, GO batches for a
    // SQL Server script that uses them. See SqlDialect::splitScript.
    QList<QAbstractItemModel *> runScriptPaged(const QString &script,
                                               QStringList *errors = nullptr) const;

    // A value that changes whenever the Schema does: SQLite's own change
    // counter, or a digest of a server's catalog. Comparing it either side of
    // a Script says whether the Schema actually changed -- which a statement
    // cannot be read off its text: a CREATE inside a transaction that rolls
    // back changes nothing, and ALTER changes plenty.
    // Empty when it could not be read.
    [[nodiscard]] QString schemaFingerprint() const;

    [[nodiscard]] QAbstractItemModel *previewTablePaged(const QString &tableName,
                                                        QString *error = nullptr,
                                                        const QString &schema = {}) const;

private:
    IDatabase *database;
};

#endif // QUERYEXECUTOR_H
