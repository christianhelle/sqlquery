#ifndef PROVIDERDATABASE_H
#define PROVIDERDATABASE_H

#include <QString>

#include "sqldatabaseadapter.h"

// Production adapter: a Database reached through any Provider -- a SQLite file
// or a PostgreSQL, SQL Server or MySQL server. Each instance owns a Qt
// connection of its own name, so two of them never share one by accident.
class ProviderDatabase final : public SqlDatabaseAdapter {
public:
    ProviderDatabase();

    ~ProviderDatabase() override;

    ProviderDatabase(const ProviderDatabase &) = delete;
    ProviderDatabase &operator=(const ProviderDatabase &) = delete;

    void setConnection(const ConnectionInfo &connection) override;

    bool open() override;

private:
    void releaseConnection();

    QString connectionName;
};

// Opens the Connection on a throwaway Database and closes it again. Empty when
// it opened, else why it did not.
QString testConnection(const ConnectionInfo &connection);

#endif // PROVIDERDATABASE_H
