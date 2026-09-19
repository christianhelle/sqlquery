#include <gtest/gtest.h>

#include "database/sqldialect.h"

namespace {
    const SqlDialect &sqlite() { return SqlDialect::forProvider(Provider::Sqlite); }
    const SqlDialect &postgres() { return SqlDialect::forProvider(Provider::PostgreSql); }
    const SqlDialect &sqlServer() { return SqlDialect::forProvider(Provider::SqlServer); }
    const SqlDialect &mySql() { return SqlDialect::forProvider(Provider::MySql); }
}

TEST(SqlDialectTest, ForProviderReturnsTheMatchingDialect) {
    EXPECT_EQ(sqlite().provider(), Provider::Sqlite);
    EXPECT_EQ(postgres().provider(), Provider::PostgreSql);
    EXPECT_EQ(sqlServer().provider(), Provider::SqlServer);
    EXPECT_EQ(mySql().provider(), Provider::MySql);
}

TEST(SqlDialectTest, EachProviderNamesItsQtDriver) {
    EXPECT_EQ(sqlite().driverName(), "QSQLITE");
    EXPECT_EQ(postgres().driverName(), "QPSQL");
    EXPECT_EQ(sqlServer().driverName(), "QODBC");
    EXPECT_EQ(mySql().driverName(), "QMYSQL");
}

TEST(SqlDialectTest, AnsiDialectsDoubleAnEmbeddedDoubleQuote) {
    EXPECT_EQ(sqlite().quoteIdentifier("we\"ird"), "\"we\"\"ird\"");
    EXPECT_EQ(postgres().quoteIdentifier("we\"ird"), "\"we\"\"ird\"");
}

TEST(SqlDialectTest, SqlServerBracketsAndDoublesAClosingBracket) {
    EXPECT_EQ(sqlServer().quoteIdentifier("order"), "[order]");
    EXPECT_EQ(sqlServer().quoteIdentifier("a]b[c"), "[a]]b[c]");
}

TEST(SqlDialectTest, MySqlBackticksAndDoublesABacktick) {
    EXPECT_EQ(mySql().quoteIdentifier("order items"), "`order items`");
    EXPECT_EQ(mySql().quoteIdentifier("a`b"), "`a``b`");
}

TEST(SqlDialectTest, QualifiedNameDelimitsEachPart) {
    EXPECT_EQ(postgres().qualifiedName("public", "my table"), "\"public\".\"my table\"");
    EXPECT_EQ(sqlServer().qualifiedName("dbo", "Orders"), "[dbo].[Orders]");
}

TEST(SqlDialectTest, QualifiedNameOmitsAnEmptySchema) {
    EXPECT_EQ(sqlite().qualifiedName("", "t"), "\"t\"");
    EXPECT_EQ(mySql().qualifiedName("", "t"), "`t`");
}

TEST(SqlDialectTest, LiteralsDoubleASingleQuote) {
    EXPECT_EQ(sqlite().quoteLiteral("O'Brien"), "'O''Brien'");
    EXPECT_EQ(postgres().quoteLiteral("O'Brien"), "'O''Brien'");
}

TEST(SqlDialectTest, SqlServerLiteralsAreUnicode) {
    EXPECT_EQ(sqlServer().quoteLiteral("O'Brien"), "N'O''Brien'");
}

TEST(SqlDialectTest, MySqlLiteralsEscapeABackslash) {
    EXPECT_EQ(mySql().quoteLiteral("C:\\dir's"), "'C:\\\\dir''s'");
}

TEST(SqlDialectTest, SelectAllCapsWithLimit) {
    EXPECT_EQ(sqlite().selectAll("\"t\"", 10), "SELECT * FROM \"t\" LIMIT 10");
    EXPECT_EQ(postgres().selectAll("\"t\"", 10), "SELECT * FROM \"t\" LIMIT 10");
    EXPECT_EQ(mySql().selectAll("`t`", 10), "SELECT * FROM `t` LIMIT 10");
}

TEST(SqlDialectTest, SqlServerCapsWithTop) {
    EXPECT_EQ(sqlServer().selectAll("[t]", 10), "SELECT TOP (10) * FROM [t]");
}

TEST(SqlDialectTest, SelectAllIsUncappedWithoutALimit) {
    EXPECT_EQ(sqlite().selectAll("\"t\""), "SELECT * FROM \"t\"");
    EXPECT_EQ(sqlServer().selectAll("[t]", 0), "SELECT * FROM [t]");
}

TEST(SqlDialectTest, ColumnsQueryQuotesTheTableAsALiteral) {
    EXPECT_TRUE(sqlite().columnsQuery("", "it's").contains("pragma_table_info('it''s')"));
    EXPECT_TRUE(postgres().columnsQuery("public", "t").contains("to_regclass('\"public\".\"t\"')"));
    EXPECT_TRUE(sqlServer().columnsQuery("dbo", "t").contains("OBJECT_ID(N'[dbo].[t]')"));
    EXPECT_TRUE(mySql().columnsQuery("", "t").contains("TABLE_NAME = 't'"));
}

TEST(SqlDialectTest, OnlyMySqlCannotShrink) {
    EXPECT_FALSE(sqlite().shrinkStatement().isEmpty());
    EXPECT_FALSE(postgres().shrinkStatement().isEmpty());
    EXPECT_FALSE(sqlServer().shrinkStatement().isEmpty());
    EXPECT_TRUE(mySql().shrinkStatement().isEmpty());
}

TEST(SqlServerDialectTest, ConnectionStringUsesSqlLogin) {
    ConnectionInfo info;
    info.provider = Provider::SqlServer;
    info.host = "sql1";
    info.databaseName = "Sales";
    info.user = "sa";
    info.password = "p;w}d";
    info.trustServerCertificate = true;

    EXPECT_EQ(SqlServerDialect::connectionString(info),
              "DRIVER={ODBC Driver 18 for SQL Server};SERVER={sql1,1433};DATABASE={Sales};"
              "UID={sa};PWD={p;w}}d};TrustServerCertificate=yes;");
}

TEST(SqlServerDialectTest, ConnectionStringUsesIntegratedAuth) {
    ConnectionInfo info;
    info.provider = Provider::SqlServer;
    info.host = "sql1";
    info.port = 14330;
    info.integratedAuth = true;
    info.user = "ignored";

    EXPECT_EQ(SqlServerDialect::connectionString(info),
              "DRIVER={ODBC Driver 18 for SQL Server};SERVER={sql1,14330};Trusted_Connection=yes;");
}
