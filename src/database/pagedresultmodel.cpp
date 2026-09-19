#include "pagedresultmodel.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "sqldialect.h"

#include <utility>

PagedResultModel::PagedResultModel(const QSqlDatabase &database, const SqlDialect &dialect, QString sql,
                                   QObject *parent)
    : QSqlQueryModel(parent),
      database(database),
      dialect(dialect),
      statement(std::move(sql)) {
    run(statement);
}

bool PagedResultModel::run(const QString &sql) {
    QSqlQuery query(this->database);
    if (!query.exec(sql)) {
        this->error = query.lastError().text();
        return false;
    }

    // QSqlQueryModel reads the first page here and the rest on demand.
    setQuery(std::move(query));
    if (lastError().isValid()) {
        this->error = lastError().text();
        return false;
    }

    this->error.clear();
    return true;
}

void PagedResultModel::sort(const int column, const Qt::SortOrder order) {
    if (column < 0 || column >= columnCount())
        return;

    const QString columnName = record().fieldName(column);
    if (columnName.isEmpty())
        return;

    // The alias is required by PostgreSQL, MySQL and SQL Server, and harmless
    // to SQLite.
    const QString sorted = QString("SELECT * FROM (%1) AS sorted_result ORDER BY %2 %3")
            .arg(statement,
                 dialect.quoteIdentifier(columnName),
                 order == Qt::AscendingOrder ? "ASC" : "DESC");

    // Not every statement can be wrapped in a subquery. run() leaves the model
    // untouched when it fails, so there is nothing to restore -- and re-running
    // the original would execute a statement with side effects a second time.
    run(sorted);
}
