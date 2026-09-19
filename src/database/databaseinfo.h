#ifndef DATABASEINFO
#define DATABASEINFO

#include <QtGlobal>
#include <QDateTime>
#include <qsqlquery.h>

#include "connectioninfo.h"

struct Column {
    int ordinal;
    QString name;
    QString dataType;
    bool notNull;
    QString defaultValue;
    bool primaryKey;
};

struct Index {
    QString name;
    QString column;
    bool unique;
    bool clustered;
};

struct Table {
    // The namespace the Table lives in (`public`, `dbo`). Empty for Providers
    // without one, SQLite and MySQL.
    QString schema;
    QString name;
    QList<Column> columns;
    QList<Index> indexes;
};

struct DatabaseInfo {
    Provider provider = Provider::Sqlite;
    // The file name for SQLite, the Connection's display name for a server.
    QString filename;
    // Server Providers only: where the Database lives.
    QString server;
    QString databaseName;
    QDateTime creationDate;
    QString databaseVersion;
    bool passwordProtected = false;
    qint64 size = 0;
    QList<Table> tables;
};

#endif // DATABASEINFO
