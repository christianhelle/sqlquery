#ifndef DBDATAEXPORT_H
#define DBDATAEXPORT_H

#include "dbexport.h"
#include "idatabase.h"
#include "../threading/cancellation.h"

#include <utility>

#include "progress.h"

class DbDataExport : public DbExport {
public:
    explicit DbDataExport(DatabaseInfo info) :
        DbExport(std::move(info)) {
    }

    void exportDataToSqlFile(IDatabase *database,
                             const QString &filename,
                             const CancellationToken *cancellationToken,
                             ExportDataProgress *progress) const;

    void exportDataToCsvFile(IDatabase *database,
                             const QString &outputFolder,
                             const QString &delimiter,
                             const CancellationToken *cancellationToken,
                             ExportDataProgress *progress) const;

private:
    // The CSV header row: Column names as the user wrote them. A CSV header
    // is not SQL, so these are not Identifiers.
    static QStringList columnNames(const Table &table);

    // The column list of an INSERT statement, where the same names are
    // Identifiers and have to be delimited.
    [[nodiscard]] QStringList quotedColumnNames(const Table &table) const;

    [[nodiscard]] QList<bool> getTextColumnFlags(const Table &table) const;

    // One CSV row: text columns in double quotes, the rest as they read.
    static QStringList getColumnValueDefs(const QList<bool> &isTextColumn,
                                          const QList<QVariant> &values);

    // The VALUES of one INSERT: NULL as NULL, numbers bare, everything else a
    // string literal in the Provider's own form. A double-quoted value is an
    // Identifier to PostgreSQL and SQL Server, so it cannot stand in for one.
    [[nodiscard]] QStringList sqlValues(const QList<bool> &isTextColumn,
                                        const QList<QVariant> &values) const;
};

#endif // DBDATAEXPORT_H
