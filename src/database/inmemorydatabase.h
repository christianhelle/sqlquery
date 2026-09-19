#ifndef INMEMORYDATABASE_H
#define INMEMORYDATABASE_H

#include <QString>

#include "sqldatabaseadapter.h"

// Test adapter: a SQLite database held in memory.
class InMemoryDatabase final : public SqlDatabaseAdapter {
public:
    InMemoryDatabase();

    void setConnection(const ConnectionInfo &connection) override;

    bool open() override;
};

#endif // INMEMORYDATABASE_H
