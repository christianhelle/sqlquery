#include <gtest/gtest.h>

#include "database/connectioninfo.h"

TEST(ConnectionInfoTest, BarePathIsASqliteFile) {
    const auto info = ConnectionInfo::fromUrl("/home/me/data.db");
    EXPECT_EQ(info.provider, Provider::Sqlite);
    EXPECT_EQ(info.filePath, "/home/me/data.db");
}

TEST(ConnectionInfoTest, SqliteFileBuildsAFileConnection) {
    const auto info = ConnectionInfo::sqliteFile("x.db");
    EXPECT_TRUE(info.isFile());
    EXPECT_EQ(info.filePath, "x.db");
}

TEST(ConnectionInfoTest, WindowsPathIsNotMistakenForAScheme) {
    const auto info = ConnectionInfo::fromUrl("C:\\data\\app.db");
    EXPECT_EQ(info.provider, Provider::Sqlite);
    EXPECT_EQ(info.filePath, "C:\\data\\app.db");
}

TEST(ConnectionInfoTest, SqliteSchemeIsStripped) {
    EXPECT_EQ(ConnectionInfo::fromUrl("sqlite:///tmp/a.db").filePath, "/tmp/a.db");
    EXPECT_EQ(ConnectionInfo::fromUrl("sqlite:a.db").filePath, "a.db");
}

TEST(ConnectionInfoTest, SqliteRoundTripsAsItsPath) {
    ConnectionInfo info;
    info.filePath = "/tmp/a b.db";
    EXPECT_EQ(info.toUrl(), "/tmp/a b.db");
    EXPECT_TRUE(ConnectionInfo::fromUrl(info.toUrl()).sameTarget(info));
}

TEST(ConnectionInfoTest, ParsesAPostgresUrl) {
    const auto info = ConnectionInfo::fromUrl("postgres://alice:s3cret@db.local:6543/shop");
    EXPECT_EQ(info.provider, Provider::PostgreSql);
    EXPECT_EQ(info.host, "db.local");
    EXPECT_EQ(info.port, 6543);
    EXPECT_EQ(info.user, "alice");
    EXPECT_EQ(info.password, "s3cret");
    EXPECT_EQ(info.databaseName, "shop");
}

TEST(ConnectionInfoTest, AcceptsSchemeAliases) {
    EXPECT_EQ(ConnectionInfo::fromUrl("postgresql://h/d").provider, Provider::PostgreSql);
    EXPECT_EQ(ConnectionInfo::fromUrl("sqlserver://h/d").provider, Provider::SqlServer);
    EXPECT_EQ(ConnectionInfo::fromUrl("mariadb://h/d").provider, Provider::MySql);
}

TEST(ConnectionInfoTest, NeverWritesThePassword) {
    auto info = ConnectionInfo::fromUrl("mysql://bob:hunter2@localhost/app");
    ASSERT_EQ(info.password, "hunter2");
    EXPECT_FALSE(info.toUrl().contains("hunter2"));
    EXPECT_EQ(info.toUrl(), "mysql://bob@localhost/app");
}

TEST(ConnectionInfoTest, SqlServerOptionsRoundTrip) {
    ConnectionInfo info;
    info.provider = Provider::SqlServer;
    info.host = "sql1";
    info.databaseName = "Sales";
    info.integratedAuth = true;
    info.trustServerCertificate = true;
    info.odbcDriver = "ODBC Driver 17 for SQL Server";

    const auto back = ConnectionInfo::fromUrl(info.toUrl());
    EXPECT_EQ(back.provider, Provider::SqlServer);
    EXPECT_EQ(back.host, "sql1");
    EXPECT_EQ(back.databaseName, "Sales");
    EXPECT_TRUE(back.integratedAuth);
    EXPECT_TRUE(back.trustServerCertificate);
    EXPECT_EQ(back.odbcDriver, "ODBC Driver 17 for SQL Server");
}

TEST(ConnectionInfoTest, DefaultOdbcDriverIsNotWritten) {
    ConnectionInfo info;
    info.provider = Provider::SqlServer;
    info.host = "sql1";
    EXPECT_EQ(info.toUrl(), "mssql://sql1");
    EXPECT_EQ(ConnectionInfo::fromUrl(info.toUrl()).odbcDriver, ConnectionInfo::defaultOdbcDriver());
}

TEST(ConnectionInfoTest, EffectivePortFallsBackToProviderDefault) {
    ConnectionInfo info;
    info.provider = Provider::PostgreSql;
    EXPECT_EQ(info.effectivePort(), 5432);
    info.provider = Provider::SqlServer;
    EXPECT_EQ(info.effectivePort(), 1433);
    info.provider = Provider::MySql;
    EXPECT_EQ(info.effectivePort(), 3306);
    info.port = 13306;
    EXPECT_EQ(info.effectivePort(), 13306);
}

TEST(ConnectionInfoTest, DisplayNameNamesProviderAndTarget) {
    const auto info = ConnectionInfo::fromUrl("postgres://alice@db:5432/shop");
    EXPECT_EQ(info.displayName(), "PostgreSQL: alice@db/shop");
}

TEST(ConnectionInfoTest, EmptyMeansNothingToConnectTo) {
    EXPECT_TRUE(ConnectionInfo().isEmpty());
    EXPECT_TRUE(ConnectionInfo::fromUrl("postgres:///db").isEmpty());
    EXPECT_FALSE(ConnectionInfo::fromUrl("a.db").isEmpty());
}
