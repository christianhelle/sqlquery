#ifndef IDATABASE_H
#define IDATABASE_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>

#include "connectioninfo.h"
#include "queryresult.h"

class QAbstractItemModel;
class SqlDialect;

class IDatabase {
public:
    virtual ~IDatabase() = default;

    // Points the Database at a Connection. Does not open it.
    virtual void setConnection(const ConnectionInfo &connection) = 0;

    [[nodiscard]] virtual ConnectionInfo connection() const = 0;

    // How SQL is written for this Database's Provider.
    [[nodiscard]] virtual const SqlDialect &dialect() const = 0;

    virtual bool open() = 0;

    virtual void close() = 0;

    // Why the last open() failed; empty when it did not.
    [[nodiscard]] virtual QString lastError() const = 0;

    // Whether the Provider can reclaim unused space at all.
    [[nodiscard]] virtual bool canShrink() const = 0;

    virtual void shrink() = 0;

    virtual QueryResult runStatement(const QString &sql) = 0;

    // A lazily fetched, pageable model over the statement's result set. Rows
    // are read in pages as they are scrolled into view, so a result set of any
    // size can be browsed. The caller owns the returned model.
    // Returns nullptr when the statement produces no result set (an INSERT,
    // say) or when it failed -- `error` then holds the reason, and is cleared
    // otherwise. A SELECT matching no rows still returns a model, so that its
    // columns can be displayed.
    virtual QAbstractItemModel *createResultModel(const QString &sql,
                                                  QString *error = nullptr) = 0;

    virtual QueryResult streamRows(const QString &sql,
                                   const std::function<bool(const QList<QVariant> &)> &onRow) = 0;
};

#endif // IDATABASE_H
