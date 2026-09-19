#ifndef SETTINGS_H
#define SETTINGS_H

#include <QPoint>
#include <QSizeF>

struct WindowState {
    QSize size;
    QPoint position;
    int treeWidth{};
    int tabWidth{};
    int queryTextHeight{};
    int queryResultHeight{};
    int editorZoomStep{};
    int treeZoomStep{};
};

struct SessionState {
    // The last-opened Connection, as ConnectionInfo::toUrl writes it.
    QString connection;
    QString query;
    QString lastUsedExportPath;
};

class Settings {
public:
    static void init();

    static QString getSettingsFolder();

    static void getMainWindowState(WindowState *state);

    static void setMainWindowState(const WindowState &state);

    static void getSessionState(SessionState *state);

    static void setSessionState(const QString &connection,
                                const QString &query);

    static void setLastUsedExportPath(const QString &path);
};


#endif //SETTINGS_H
