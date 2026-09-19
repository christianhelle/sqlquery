#include "connectioninfo.h"

#include <QUrl>
#include <QUrlQuery>

QString providerName(const Provider provider) {
    switch (provider) {
        case Provider::Sqlite:
            return QStringLiteral("SQLite");
        case Provider::PostgreSql:
            return QStringLiteral("PostgreSQL");
        case Provider::SqlServer:
            return QStringLiteral("SQL Server");
        case Provider::MySql:
            return QStringLiteral("MySQL");
    }
    return {};
}

int defaultPort(const Provider provider) {
    switch (provider) {
        case Provider::Sqlite:
            return 0;
        case Provider::PostgreSql:
            return 5432;
        case Provider::SqlServer:
            return 1433;
        case Provider::MySql:
            return 3306;
    }
    return 0;
}

namespace {
    QString schemeFor(const Provider provider) {
        switch (provider) {
            case Provider::PostgreSql:
                return QStringLiteral("postgres");
            case Provider::SqlServer:
                return QStringLiteral("mssql");
            case Provider::MySql:
                return QStringLiteral("mysql");
            case Provider::Sqlite:
                break;
        }
        return QStringLiteral("sqlite");
    }

    bool providerForScheme(const QString &scheme, Provider *provider) {
        const QString s = scheme.toLower();
        if (s == "postgres" || s == "postgresql") {
            *provider = Provider::PostgreSql;
        } else if (s == "mssql" || s == "sqlserver") {
            *provider = Provider::SqlServer;
        } else if (s == "mysql" || s == "mariadb") {
            *provider = Provider::MySql;
        } else if (s == "sqlite") {
            *provider = Provider::Sqlite;
        } else {
            return false;
        }
        return true;
    }

    bool isTrue(const QString &value) {
        const QString v = value.toLower();
        return v == "1" || v == "true" || v == "yes";
    }
}

bool ConnectionInfo::isEmpty() const {
    return isFile() ? filePath.isEmpty() : host.isEmpty();
}

int ConnectionInfo::effectivePort() const {
    return port > 0 ? port : defaultPort(provider);
}

QString ConnectionInfo::displayName() const {
    if (isFile())
        return filePath;

    QString target = host;
    if (port > 0 && port != defaultPort(provider))
        target += ":" + QString::number(port);
    if (!databaseName.isEmpty())
        target += "/" + databaseName;
    if (!user.isEmpty() && !integratedAuth)
        target = user + "@" + target;
    return providerName(provider) + ": " + target;
}

QString ConnectionInfo::toUrl() const {
    if (isFile())
        return filePath;

    QUrl url;
    url.setScheme(schemeFor(provider));
    url.setHost(host);
    if (port > 0)
        url.setPort(port);
    if (!user.isEmpty() && !integratedAuth)
        url.setUserName(user);
    if (!databaseName.isEmpty())
        url.setPath("/" + databaseName);

    if (provider == Provider::SqlServer) {
        QUrlQuery query;
        if (odbcDriver != defaultOdbcDriver())
            query.addQueryItem("driver", odbcDriver);
        if (trustServerCertificate)
            query.addQueryItem("trust", "1");
        if (integratedAuth)
            query.addQueryItem("integrated", "1");
        url.setQuery(query);
    }
    return url.toString(QUrl::FullyEncoded);
}

ConnectionInfo ConnectionInfo::fromUrl(const QString &text) {
    ConnectionInfo info;
    const QString trimmed = text.trimmed();

    // A single-letter scheme is a Windows drive, not a Provider.
    const int colon = trimmed.indexOf(':');
    Provider provider;
    if (colon <= 1 || !providerForScheme(trimmed.left(colon), &provider)) {
        info.filePath = trimmed;
        return info;
    }

    if (provider == Provider::Sqlite) {
        QString path = trimmed.mid(colon + 1);
        if (path.startsWith("//"))
            path = QUrl(trimmed).path();
        info.filePath = path;
        return info;
    }

    const QUrl url(trimmed, QUrl::TolerantMode);
    info.provider = provider;
    info.host = url.host();
    info.port = url.port(0);
    info.user = url.userName();
    info.password = url.password();
    info.databaseName = url.path().mid(1);

    const QUrlQuery query(url);
    if (query.hasQueryItem("driver"))
        info.odbcDriver = query.queryItemValue("driver", QUrl::FullyDecoded);
    info.trustServerCertificate = isTrue(query.queryItemValue("trust"));
    info.integratedAuth = isTrue(query.queryItemValue("integrated"));
    return info;
}

bool ConnectionInfo::sameTarget(const ConnectionInfo &other) const {
    return toUrl() == other.toUrl();
}
