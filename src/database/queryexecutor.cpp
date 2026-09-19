#include "queryexecutor.h"

#include "sqldialect.h"

QueryExecutor::QueryExecutor(IDatabase *database)
    : database(database) {
}

QList<QueryResult> QueryExecutor::runScript(const QString &script, QStringList *errors) {
    const QStringList raw = script.split(";", Qt::SkipEmptyParts);
    return runStatements(raw, errors);
}

QList<QueryResult> QueryExecutor::runStatements(const QStringList &statements,
                                               QStringList *errors) {
    QList<QueryResult> results;
    results.reserve(statements.size());
    for (const auto &raw : statements) {
        // Newlines are left alone: SQLite treats them as whitespace, and
        // stripping them would merge tokens across lines and swallow the rest
        // of a line after a `--` comment.
        const QString sql = raw.trimmed();
        if (sql.isEmpty())
            continue;

        QueryResult result = database->runStatement(sql);
        if (!result.ok) {
            if (errors != nullptr)
                errors->append(result.error.isEmpty() ? QString("Query failed") : result.error);
        }
        results.append(std::move(result));
    }
    return results;
}

QueryResult QueryExecutor::previewTable(const QString &tableName, const int limit,
                                        const QString &schema) const {
    const SqlDialect &dialect = database->dialect();
    return database->runStatement(dialect.selectAll(dialect.qualifiedName(schema, tableName), limit));
}

QList<QAbstractItemModel *> QueryExecutor::runScriptPaged(const QString &script,
                                                          QStringList *errors) const {
    return runStatementsPaged(script.split(";", Qt::SkipEmptyParts), errors);
}

QString QueryExecutor::schemaFingerprint() const {
    const QueryResult result = database->runStatement(database->dialect().schemaFingerprintQuery());
    if (!result.ok || result.rows.isEmpty() || result.rows.first().values.isEmpty())
        return {};

    return result.rows.first().values.first().toString();
}

QueryResult QueryExecutor::dropTable(const QString &tableName, const QString &schema) const {
    return database->runStatement(
        QString("DROP TABLE %1").arg(database->dialect().qualifiedName(schema, tableName)));
}

QList<QAbstractItemModel *> QueryExecutor::runStatementsPaged(const QStringList &statements,
                                                             QStringList *errors) const {
    QList<QAbstractItemModel *> models;
    for (const auto &raw : statements) {
        const QString sql = raw.trimmed();
        if (sql.isEmpty())
            continue;

        QString error;
        // Statements that return no rows still run; they just have nothing to show.
        if (auto *model = database->createResultModel(sql, &error)) {
            models.append(model);
        } else if (!error.isEmpty() && errors != nullptr) {
            errors->append(error);
        }
    }
    return models;
}

QAbstractItemModel *QueryExecutor::previewTablePaged(const QString &tableName,
                                                     QString *error,
                                                     const QString &schema) const {
    const SqlDialect &dialect = database->dialect();
    return database->createResultModel(dialect.selectAll(dialect.qualifiedName(schema, tableName)), error);
}
