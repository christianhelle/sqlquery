#include "script.h"

#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

#include "../database/providerdatabase.h"
#include "../database/queryexecutor.h"

void Script::executeSqlFile(const QString &sqlFilePath,
                            const ConnectionInfo &connection) {
    const auto sqlFile = std::make_unique<QFile>(sqlFilePath);
    if (!sqlFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QElapsedTimer time;
    time.start();
    const QString sqlScript = QTextStream(sqlFile.get()).readAll();

    const auto database = std::make_unique<ProviderDatabase>();
    database->setConnection(connection);
    if (!database->open()) {
        qWarning().noquote() << "Unable to open" << connection.displayName() << "-" << database->lastError();
        return;
    }

    QueryExecutor executor(database.get());
    QStringList errors;
    executor.runScript(sqlScript, &errors);

    const auto milliseconds = static_cast<double>(time.elapsed());
    const auto msg = "Script execution took " + QString::number(milliseconds / 1000) + " seconds";
    QTextStream out(stdout);
    out << msg;
    fflush(stdout);
}
