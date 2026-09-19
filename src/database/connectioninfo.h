#ifndef CONNECTIONINFO_H
#define CONNECTIONINFO_H

#include <QString>

// The database engine behind a Connection.
enum class Provider { Sqlite, PostgreSql, SqlServer, MySql };

QString providerName(Provider provider);

// The port a server Provider listens on when none is given. Zero for SQLite,
// which has no server.
int defaultPort(Provider provider);

// How to reach a Database: a file for SQLite, a server and database for the
// rest. The password is carried for the open but never written out -- toUrl
// leaves it behind, so nothing that persists a Connection can leak it.
struct ConnectionInfo {
    Provider provider = Provider::Sqlite;
    QString filePath;
    QString host;
    int port = 0;
    QString databaseName;
    QString user;
    QString password;
    // SQL Server only: authenticate as the signed-in user instead of user/password.
    bool integratedAuth = false;
    // SQL Server only: the ODBC driver the connection goes through.
    QString odbcDriver = defaultOdbcDriver();
    // SQL Server only: accept a self-signed server certificate.
    bool trustServerCertificate = false;

    // A SQLite Connection to the file at `path`.
    static ConnectionInfo sqliteFile(const QString &path) {
        ConnectionInfo info;
        info.filePath = path;
        return info;
    }

    static QString defaultOdbcDriver() { return QStringLiteral("ODBC Driver 18 for SQL Server"); }

    [[nodiscard]] bool isFile() const { return provider == Provider::Sqlite; }

    // Nothing to connect to yet: no file for SQLite, no host for a server.
    [[nodiscard]] bool isEmpty() const;

    // The port to connect on: the given one, else the Provider's default.
    [[nodiscard]] int effectivePort() const;

    // What the user reads in the window title and the recent list.
    [[nodiscard]] QString displayName() const;

    // A single-line form used for recents, the session and the command line:
    // a bare file path for SQLite, else `postgres://user@host:port/database`,
    // `mysql://...` or `mssql://...?driver=...&trust=1&integrated=1`.
    // Never holds the password.
    [[nodiscard]] QString toUrl() const;

    // Reads what toUrl writes. A URL may also carry a password, for the command
    // line. Anything without a known scheme is taken as a SQLite file path, so a
    // Windows path like C:\data.db stays a path.
    static ConnectionInfo fromUrl(const QString &text);

    // Same target and credentials identity; the password is not compared.
    [[nodiscard]] bool sameTarget(const ConnectionInfo &other) const;
};

#endif // CONNECTIONINFO_H
