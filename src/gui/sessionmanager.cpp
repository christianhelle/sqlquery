#include "sessionmanager.h"
#include "../settings/recentconnections.h"
#include "../settings/settings.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

SessionManager::SessionManager(QObject *parent) : QObject(parent) {
}

void SessionManager::init() {
    Settings::init();
}

QString SessionManager::getSettingsFolder() {
    return Settings::getSettingsFolder();
}

void SessionManager::saveSession(const QString &connection, const QString &query) {
    Settings::setSessionState(connection, query);
}

void SessionManager::restoreSession(SessionState *state) const {
    Settings::getSessionState(state);
}

void SessionManager::saveWindowState(const WindowState &state) {
    Settings::setMainWindowState(state);
}

void SessionManager::restoreWindowState(WindowState *state) const {
    Settings::getMainWindowState(state);
}

void SessionManager::loadRecentConnections(QMenu *menu, QObject *parent) const {
    QStringList connections = RecentConnections::getList();
    if (connections.isEmpty())
        return;

    if (!menu->actions().isEmpty())
        menu->clear();

    foreach (const QString &connection, connections) {
        QAction *action = menu->addAction(connection);
        action->setObjectName(connection);
        connect(action, SIGNAL(triggered(bool)), parent, SLOT(openRecentConnection()));
    }
}

void SessionManager::addRecentConnection(const QString &connection) const {
    RecentConnections::add(connection);
}

QStringList SessionManager::getRecentConnections() const {
    return RecentConnections::getList();
}

void SessionManager::setLastUsedExportPath(const QString &path) {
    Settings::setLastUsedExportPath(path);
}

QString SessionManager::getLastUsedExportPath() const {
    SessionState state;
    Settings::getSessionState(&state);
    return state.lastUsedExportPath;
}
