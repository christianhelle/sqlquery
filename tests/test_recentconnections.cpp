#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "settings/recentconnections.h"
#include "settings/settings.h"

// test_main points HOME at a throwaway directory, so the recents file written
// here never touches the settings of whoever runs the tests.
class RecentConnectionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Settings::init();
        RecentConnections::clear();
    }

    void TearDown() override {
        RecentConnections::clear();
    }
};

TEST_F(RecentConnectionsTest, KeepsAServerUrl) {
    RecentConnections::add("postgres://alice@db/shop");

    EXPECT_TRUE(RecentConnections::getList().contains("postgres://alice@db/shop"));
}

TEST_F(RecentConnectionsTest, KeepsAnExistingFile) {
    QTemporaryDir dir;
    const QString path = dir.path() + "/a.db";
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();

    RecentConnections::add(path);

    EXPECT_TRUE(RecentConnections::getList().contains(QDir::toNativeSeparators(path)));
}

TEST_F(RecentConnectionsTest, SkipsAFileThatDoesNotExist) {
    RecentConnections::add("/no/such/file.db");

    EXPECT_TRUE(RecentConnections::getList().isEmpty());
}

TEST_F(RecentConnectionsTest, SkipsAServerUrlWithoutAHost) {
    RecentConnections::add("mysql:///app");

    EXPECT_TRUE(RecentConnections::getList().isEmpty());
}
