#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <memory>

#include "database/dbanalyzer.h"
#include "database/dbexportdata.h"
#include "database/dbexportschema.h"
#include "database/providerdatabase.h"
#include "database/queryexecutor.h"
#include "database/sqldialect.h"

// Runs the database module against a real server. Each Provider is skipped
// unless its connection URL is set, password included:
//   SQLQUERY_TEST_PG     postgres://postgres:pw@localhost:5432/postgres
//   SQLQUERY_TEST_MSSQL  mssql://sa:pw@localhost:1433/master?trust=1
//   SQLQUERY_TEST_MYSQL  mysql://root:pw@localhost:3306/test
// tests/docker-compose.yml starts all three.

namespace {
    struct ProviderCase {
        const char *name;
        const char *variable;
        // Where the test Table lives: the Provider's default Schema.
        const char *schema;
    };

    void PrintTo(const ProviderCase &c, std::ostream *os) { *os << c.name; }

    const QString TableName = "sq_it_orders";
}

class ProviderTest : public ::testing::TestWithParam<ProviderCase> {
protected:
    void SetUp() override {
        const QString url = qEnvironmentVariable(GetParam().variable);
        if (url.isEmpty())
            GTEST_SKIP() << GetParam().variable << " is not set";

        connection = ConnectionInfo::fromUrl(url);
        db = std::make_unique<ProviderDatabase>();
        db->setConnection(connection);
        ASSERT_TRUE(db->open()) << db->lastError().toStdString();

        executor = std::make_unique<QueryExecutor>(db.get());
        drop();
        const auto created = run(QString(
                "CREATE TABLE %1 (id INTEGER PRIMARY KEY, name VARCHAR(50) NOT NULL, "
                "price DECIMAL(10,2), note VARCHAR(100))").arg(table()));
        ASSERT_TRUE(created.ok) << created.error.toStdString();
        for (const auto &insert: {
                 QString("INSERT INTO %1 (id, name, price, note) VALUES (1, 'Widget', 9.99, 'it''s \"fine\"')"),
                 QString("INSERT INTO %1 (id, name, price, note) VALUES (2, 'Gadget', 24.50, NULL)"),
                 QString("INSERT INTO %1 (id, name, price, note) VALUES (3, 'Apple', 1.00, 'a, b')")}) {
            const auto inserted = run(insert.arg(table()));
            ASSERT_TRUE(inserted.ok) << inserted.error.toStdString();
        }
    }

    void TearDown() override {
        if (db) {
            drop();
            db->close();
        }
    }

    [[nodiscard]] QString schema() const { return GetParam().schema; }

    [[nodiscard]] QString table() const { return db->dialect().qualifiedName(schema(), TableName); }

    QueryResult run(const QString &sql) const { return db->runStatement(sql); }

    void drop() const { executor->dropTable(TableName, schema()); }

    [[nodiscard]] DatabaseInfo analyze() const {
        DatabaseInfo info;
        EXPECT_TRUE(DbAnalyzer(db.get()).analyze(info));
        return info;
    }

    [[nodiscard]] static const Table *find(const DatabaseInfo &info, const QString &name) {
        for (const auto &t: info.tables) {
            if (t.name == name)
                return &t;
        }
        return nullptr;
    }

    [[nodiscard]] DatabaseInfo onlyTestTable() const {
        DatabaseInfo info = analyze();
        const Table *t = find(info, TableName);
        DatabaseInfo only = info;
        only.tables.clear();
        if (t != nullptr)
            only.tables << *t;
        return only;
    }

    ConnectionInfo connection;
    std::unique_ptr<ProviderDatabase> db;
    std::unique_ptr<QueryExecutor> executor;
};

TEST_P(ProviderTest, AnalyzerFindsTheTableAndItsColumns) {
    const DatabaseInfo info = analyze();
    EXPECT_EQ(info.provider, connection.provider);
    EXPECT_FALSE(info.databaseVersion.isEmpty());

    const Table *t = find(info, TableName);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->schema, schema());
    ASSERT_EQ(t->columns.size(), 4);

    EXPECT_EQ(t->columns.at(0).name, "id");
    EXPECT_TRUE(t->columns.at(0).primaryKey);
    EXPECT_EQ(t->columns.at(1).name, "name");
    EXPECT_TRUE(t->columns.at(1).notNull);
    EXPECT_FALSE(t->columns.at(1).primaryKey);
    EXPECT_TRUE(t->columns.at(1).dataType.contains("50")) << t->columns.at(1).dataType.toStdString();
    EXPECT_FALSE(t->columns.at(3).notNull);
}

TEST_P(ProviderTest, PreviewHonoursTheLimit) {
    const QueryResult all = executor->previewTable(TableName, -1, schema());
    ASSERT_TRUE(all.ok) << all.error.toStdString();
    EXPECT_EQ(all.rows.size(), 3);

    const QueryResult two = executor->previewTable(TableName, 2, schema());
    ASSERT_TRUE(two.ok) << two.error.toStdString();
    EXPECT_EQ(two.rows.size(), 2);
}

TEST_P(ProviderTest, PagedResultSortsOnTheServer) {
    QString error;
    std::unique_ptr<QAbstractItemModel> model(executor->previewTablePaged(TableName, &error, schema()));
    ASSERT_NE(model, nullptr) << error.toStdString();

    model->sort(1, Qt::AscendingOrder);
    EXPECT_EQ(model->index(0, 1).data().toString(), "Apple");
    model->sort(1, Qt::DescendingOrder);
    EXPECT_EQ(model->index(0, 1).data().toString(), "Widget");
}

TEST_P(ProviderTest, FingerprintChangesWithTheSchemaOnly) {
    const QString before = executor->schemaFingerprint();
    ASSERT_FALSE(before.isEmpty());

    ASSERT_TRUE(executor->previewTable(TableName, 1, schema()).ok);
    EXPECT_EQ(executor->schemaFingerprint(), before);

    ASSERT_TRUE(run(QString("ALTER TABLE %1 ADD extra INTEGER").arg(table())).ok);
    EXPECT_NE(executor->schemaFingerprint(), before);
}

TEST_P(ProviderTest, SqlExportReplaysIntoTheSameTable) {
    QTemporaryDir dir;
    const QString path = dir.path() + "/export.sql";
    CancellationTokenSource tcs;
    const CancellationToken token = tcs.get();
    ExportDataProgress progress;
    DbDataExport(onlyTestTable()).exportDataToSqlFile(db.get(), path, &token, &progress);
    EXPECT_EQ(progress.getAffectedRows(), 3u);

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString script = QTextStream(&file).readAll();

    ASSERT_TRUE(run(QString("DELETE FROM %1").arg(table())).ok);
    QStringList errors;
    executor->runScript(script, &errors);
    EXPECT_TRUE(errors.isEmpty()) << errors.join("\n").toStdString() << "\n" << script.toStdString();

    const QueryResult note = run(QString("SELECT note FROM %1 WHERE id = 1").arg(table()));
    ASSERT_TRUE(note.ok) << note.error.toStdString();
    ASSERT_EQ(note.rows.size(), 1);
    EXPECT_EQ(note.rows.first().values.first().toString(), "it's \"fine\"");

    const QueryResult nulls = run(QString("SELECT note FROM %1 WHERE id = 2").arg(table()));
    ASSERT_EQ(nulls.rows.size(), 1);
    EXPECT_TRUE(nulls.rows.first().values.first().isNull());
}

TEST_P(ProviderTest, CsvExportWritesOneFilePerTable) {
    QTemporaryDir dir;
    CancellationTokenSource tcs;
    const CancellationToken token = tcs.get();
    ExportDataProgress progress;
    DbDataExport(onlyTestTable()).exportDataToCsvFile(db.get(), dir.path(), ",", &token, &progress);

    const QString baseName = schema().isEmpty() ? TableName : schema() + "." + TableName;
    QFile file(dir.path() + "/" + baseName + ".csv");
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QTextStream(&file).readAll();
    EXPECT_TRUE(content.startsWith("id,name,price,note"));
    EXPECT_TRUE(content.contains("\"a, b\""));
}

TEST_P(ProviderTest, SchemaExportNamesTheQualifiedTable) {
    const QString sql = DbSchemaExport(onlyTestTable()).exportSchema();
    EXPECT_TRUE(sql.contains("CREATE TABLE " + table())) << sql.toStdString();
}

TEST_P(ProviderTest, DropTableRemovesIt) {
    ASSERT_TRUE(executor->dropTable(TableName, schema()).ok);
    EXPECT_EQ(find(analyze(), TableName), nullptr);
}

// A procedure body holds semicolons and has to reach SQL Server as one batch,
// which is what GO marks out. GO with a count runs its batch that many times.
TEST_P(ProviderTest, SqlServerScriptRunsGoBatches) {
    if (connection.provider != Provider::SqlServer)
        GTEST_SKIP() << "GO batches are SQL Server only";

    const QString script = QString(
            "DROP PROCEDURE IF EXISTS dbo.sq_it_count;\n"
            "GO\n"
            "CREATE PROCEDURE dbo.sq_it_count AS\n"
            "BEGIN\n"
            "    SET NOCOUNT ON;\n"
            "    SELECT COUNT(*) AS n FROM %1;\n"
            "END\n"
            "GO\n"
            "INSERT INTO %1 (id, name) SELECT MAX(id) + 1, 'Repeat' FROM %1\n"
            "GO 2\n").arg(table());

    QStringList errors;
    executor->runScript(script, &errors);
    EXPECT_TRUE(errors.isEmpty()) << errors.join("\n").toStdString();

    const QueryResult counted = run("EXEC dbo.sq_it_count");
    ASSERT_TRUE(counted.ok) << counted.error.toStdString();
    ASSERT_EQ(counted.rows.size(), 1);
    EXPECT_EQ(counted.rows.first().values.first().toInt(), 5);

    run("DROP PROCEDURE IF EXISTS dbo.sq_it_count");
}

INSTANTIATE_TEST_SUITE_P(Servers, ProviderTest,
                         ::testing::Values(ProviderCase{"PostgreSql", "SQLQUERY_TEST_PG", "public"},
                                           ProviderCase{"SqlServer", "SQLQUERY_TEST_MSSQL", "dbo"},
                                           ProviderCase{"MySql", "SQLQUERY_TEST_MYSQL", ""}),
                         [](const auto &info) { return std::string(info.param.name); });
