#ifndef SQLDATABASEADAPTER_H
#define SQLDATABASEADAPTER_H

#include <QSqlDatabase>
#include <QString>

#include "idatabase.h"
#include "sqldialect.h"

// Shared base for the IDatabase adapters. They differ only in how their
// connection is named, created and opened; everything that reads from an open
// connection is the same for all of them and lives here.
class SqlDatabaseAdapter : public IDatabase {
public:
    void close() override;

    [[nodiscard]] ConnectionInfo connection() const override { return info; }

    [[nodiscard]] const SqlDialect &dialect() const override { return *sqlDialect; }

    [[nodiscard]] QString lastError() const override;

    [[nodiscard]] bool canShrink() const override { return !sqlDialect->shrinkStatement().isEmpty(); }

    // Runs the Provider's shrink statement. Does nothing on a closed Database:
    // opening one is the job of whoever owns it.
    void shrink() override;

    QueryResult runStatement(const QString &sql) override;

    QAbstractItemModel *createResultModel(const QString &sql,
                                          QString *error = nullptr) override;

    QueryResult streamRows(const QString &sql,
                           const std::function<bool(const QList<QVariant> &)> &onRow) override;

    [[nodiscard]] QSqlDatabase getConnection() const { return database; }

protected:
    QSqlDatabase database;
    ConnectionInfo info;
    const SqlDialect *sqlDialect = &SqlDialect::forProvider(Provider::Sqlite);
};

#endif // SQLDATABASEADAPTER_H
