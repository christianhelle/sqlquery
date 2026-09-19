#include "inmemorydatabase.h"

#include <QSqlDatabase>

InMemoryDatabase::InMemoryDatabase() {
    database = QSqlDatabase::addDatabase("QSQLITE", "in_memory_connection");
    database.setDatabaseName(":memory:");
}

// Deliberately does not close first, unlike the production adapter: for an
// in-memory database the data *is* the connection, so closing would discard
// it. Nothing switches source on this adapter, and it only speaks SQLite.
void InMemoryDatabase::setConnection(const ConnectionInfo &connection) {
    this->info = ConnectionInfo::sqliteFile(connection.filePath);
    database.setDatabaseName(connection.filePath);
}

bool InMemoryDatabase::open() {
    if (database.isOpen())
        return true;
    return database.open();
}
