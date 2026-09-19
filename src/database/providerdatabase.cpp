#include "providerdatabase.h"

#include <QAtomicInteger>
#include <QSqlDatabase>

namespace {
    QString nextConnectionName() {
        static QAtomicInteger<int> counter;
        return QStringLiteral("sqlquery_%1").arg(counter.fetchAndAddRelaxed(1));
    }
}

ProviderDatabase::ProviderDatabase() :
    connectionName(nextConnectionName()) {
    database = QSqlDatabase::addDatabase(sqlDialect->driverName(), connectionName);
}

ProviderDatabase::~ProviderDatabase() {
    releaseConnection();
}

void ProviderDatabase::releaseConnection() {
    close();
    // Qt refuses to remove a connection while a handle to it is still held.
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

void ProviderDatabase::setConnection(const ConnectionInfo &connection) {
    this->close();

    const SqlDialect &next = SqlDialect::forProvider(connection.provider);
    // A connection is bound to its driver, so another Provider needs a new one.
    if (database.driverName() != next.driverName()) {
        releaseConnection();
        database = QSqlDatabase::addDatabase(next.driverName(), connectionName);
    }

    this->sqlDialect = &next;
    this->info = connection;
    next.configure(database, connection);
}

bool ProviderDatabase::open() {
    // Idempotent, as it is for the in-memory adapter: re-opening used to close
    // first, which cut short any PagedResult still reading through the
    // connection. Pointing at a different target goes through setConnection,
    // which closes the old one.
    if (database.isOpen())
        return true;

    return database.open();
}
