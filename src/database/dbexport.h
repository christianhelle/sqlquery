#ifndef DBEXPORT_H
#define DBEXPORT_H

#include "databaseinfo.h"
#include "sqldialect.h"

class DbExport {
public:
    explicit DbExport(DatabaseInfo info)
        : info(std::move(info)) {
    }

protected:
    [[nodiscard]] const DatabaseInfo &getDatabaseInfo() const { return info; }
    // How SQL is written for the Provider the DatabaseInfo was read from.
    [[nodiscard]] const SqlDialect &dialect() const { return SqlDialect::forProvider(info.provider); }
    [[nodiscard]] QString qualifiedName(const Table &table) const {
        return dialect().qualifiedName(table.schema, table.name);
    }
    [[nodiscard]] const QStringList &getTextTypes() const { return textTypes; }
    static bool isInternalTable(const Table &table);

private:
    DatabaseInfo info;
    const QStringList textTypes = {
        "TEXT",
        "CHARACTER",
        "VARCHAR",
        "VARYING CHARACTER",
        "NCHAR",
        "NATIVE CHARACTER",
        "NVARCHAR",
        "CLOB"
    };
};


#endif //DBEXPORT_H
