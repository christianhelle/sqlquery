#include "recentconnections.h"
#include "settings.h"

#include "../database/connectioninfo.h"

#include <QOperatingSystemVersion>
#include <QStringList>
#include <QFile>
#include <QTextStream>

QString RecentConnections::getRecentsFilePath() {
    return Settings::getSettingsFolder() + "/.recents";
}

// Only a file path has its separators made native; a URL is left as written.
QString RecentConnections::sanitize(const QString &connection) {
    if (!ConnectionInfo::fromUrl(connection).isFile())
        return connection;
    return QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows
               ? QString(connection).replace('\\', '/')
               : QString(connection).replace('/', '\\');
}

bool RecentConnections::isAvailable(const QString &connection) {
    const ConnectionInfo info = ConnectionInfo::fromUrl(connection);
    return !info.isEmpty() && (!info.isFile() || QFile::exists(info.filePath));
}

void RecentConnections::add(const QString &connection) {
    if (connection.isEmpty() || !isAvailable(connection))
        return;

    auto files = getList();
    const auto native_path = sanitize(connection);
    if (files.contains(native_path, Qt::CaseInsensitive)) {
        return;
    }
    files.append(native_path);

    const QString recentsFilePath = RecentConnections::getRecentsFilePath();
    const auto file = std::make_unique<QFile>(recentsFilePath);
    if (!file->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }

    QTextStream out(file.get());
    for (const auto &path : files) {
        out << path << "\n";
    }

    file->close();
}

void RecentConnections::clear() {
    QFile::remove(getRecentsFilePath());
}

QStringList RecentConnections::getList() {
    QStringList files;

    const QString filePath = RecentConnections::getRecentsFilePath();
    const auto file = std::make_unique<QFile>(filePath);
    if (!file->open(QIODevice::ReadWrite | QIODevice::Text)) {
        return files;
    }

    if (QTextStream in(file.get()); in.seek(0)) {
        while (!in.atEnd()) {
            if (const auto path = sanitize(in.readLine());
                !files.contains(path, Qt::CaseInsensitive) && isAvailable(path)) {
                files.append(path);
            }
        }
    }

    file->close();

    QStringList list;
    list.reserve(files.size());
    std::reverse_copy(files.begin(),
                      files.end(),
                      std::back_inserter(list));

    return list;
}
