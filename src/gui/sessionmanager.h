#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMenu>
#include <QSizeF>
#include <QPoint>
#include "../settings/settings.h"

class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = nullptr);

    static void init();

    // Session state
    void saveSession(const QString &connection, const QString &query);
    void restoreSession(SessionState *state) const;

    // Window state
    void saveWindowState(const WindowState &state);
    void restoreWindowState(WindowState *state) const;

    // Recent connections
    void loadRecentConnections(QMenu *menu, QObject *parent) const;
    void addRecentConnection(const QString &connection) const;
    QStringList getRecentConnections() const;

    // Export path
    void setLastUsedExportPath(const QString &path);
    QString getLastUsedExportPath() const;

private:
    static QString getSettingsFolder();
};

#endif // SESSIONMANAGER_H
