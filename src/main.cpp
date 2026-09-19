#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

#include "cli/export.h"
#include "cli/script.h"
#include "database/providerdatabase.h"
#include "gui/mainwindow.h"

constexpr auto Version = "1.0.0";

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(Version);
    QApplication::setOrganizationDomain("christianhelle.com");
    QApplication::setOrganizationName("Christian Helle");
    QApplication::setApplicationName("SQL Query Analyzer");

    QCommandLineParser parser;
    parser.setApplicationDescription(
            "A fast and lightweight cross-platform command line and GUI tool for querying and manipulating "
            "SQLite, PostgreSQL, SQL Server and MySQL databases");
    parser.addPositionalArgument("database",
                                 "SQLite file or connection URL to open, e.g. postgres://user@host:5432/db, "
                                 "mysql://user@host/db or mssql://user@host/db?trust=1");
    const QCommandLineOption progressOption(QStringList() << "p" << "progress", "Show progress during copy");
    const QCommandLineOption exportCsvOption(QStringList() << "e" << "export-csv", "Export data to CSV.");
    const QCommandLineOption targetDirectoryOption(QStringList() << "d" << "target-directory",
                                                   "Target directory for export.",
                                                   "directory");
    const QCommandLineOption importSqlOption(QStringList() << "r" << "run-sql", "Execute SQL file.", "file");
    const QCommandLineOption passwordOption(QStringList() << "password",
                                            "Password for a server connection. Defaults to the "
                                            "SQLQUERY_PASSWORD environment variable.",
                                            "password");
    const auto helpOption = parser.addHelpOption();
    const auto versionOption = parser.addVersionOption();

    parser.addOption(progressOption);
    parser.addOption(exportCsvOption);
    parser.addOption(targetDirectoryOption);
    parser.addOption(importSqlOption);
    parser.addOption(passwordOption);
    parser.process(app);

    if (parser.isSet(helpOption)) {
        parser.showHelp();
    }

    if (parser.isSet(versionOption)) {
        parser.showVersion();
    }

    const auto args = parser.positionalArguments();
    bool showProgress = parser.isSet(progressOption);
    bool exportOption = parser.isSet(exportCsvOption);
    auto importSql = parser.isSet(importSqlOption);
    auto outputFolder = parser.value(targetDirectoryOption);

    // The Connection named on the command line, with its password filled in
    // from the option or the environment when the URL does not carry one.
    const auto connectionArgument = [&]() {
        ConnectionInfo info = ConnectionInfo::fromUrl(args.at(0));
        if (!info.isFile() && info.password.isEmpty()) {
            info.password = parser.isSet(passwordOption)
                                ? parser.value(passwordOption)
                                : qEnvironmentVariable("SQLQUERY_PASSWORD");
        }
        return info;
    };

    if (exportOption) {
        if (args.length() != 1) {
            qWarning("Export option requires a database file or connection URL.");
            return 1;
        }
        if (outputFolder.isEmpty()) {
            qWarning("Target directory is required for export.");
            qWarning("Setting target directory to current working directory.");
            outputFolder = QDir::currentPath();
        }

        Export::exportDataToCsvFile(connectionArgument(), outputFolder, showProgress);
        return 0;
    }

    if (importSql){
        if (args.length() != 1) {
            qWarning("Execute SQL option requires exactly one database file or connection URL.");
            return 1;
        }
        const auto &sqlFile = parser.value(importSqlOption);
        Script::executeSqlFile(sqlFile, connectionArgument());
        return 0;
    }

    // The window works on this Database and does not outlive it.
    ProviderDatabase database;

    if (args.length() == 1) {
        MainWindow window(&database);
        const ConnectionInfo connection = connectionArgument();
        if (connection.isFile() || connection.password.isEmpty())
            window.openDatabase(args.at(0));
        else
            window.openConnection(connection);
        window.show();
        return QApplication::exec();
    }

    MainWindow window(&database);
    window.restoreLastSession();
    window.show();
    return QApplication::exec();
}
