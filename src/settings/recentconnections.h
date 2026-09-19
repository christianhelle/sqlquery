#ifndef RECENTCONNECTIONS_H
#define RECENTCONNECTIONS_H

#include <QStringList>

// The Connections the user opened lately, newest first, each held in the form
// ConnectionInfo::toUrl writes: a file path for SQLite, a URL for a server.
// A file that no longer exists drops out; a server is kept, as there is no
// telling from here whether it is reachable.
class RecentConnections {
public:
    static void add(const QString &connection);

    static void clear();

    static QStringList getList();

private:
    static QString getRecentsFilePath();

    static QString sanitize(const QString &connection);

    static bool isAvailable(const QString &connection);
};

#endif // RECENTCONNECTIONS_H
