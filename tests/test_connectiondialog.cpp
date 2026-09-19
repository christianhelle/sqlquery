#include <gtest/gtest.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "gui/connectiondialog.h"

namespace {
    template<typename T>
    T *field(const ConnectionDialog &dialog, const char *name) {
        return dialog.findChild<T *>(name);
    }

    void selectProvider(const ConnectionDialog &dialog, const Provider provider) {
        auto *combo = field<QComboBox>(dialog, "providerCombo");
        combo->setCurrentIndex(combo->findData(static_cast<int>(provider)));
    }
}

TEST(ConnectionDialogTest, StartsOnSqlite) {
    const ConnectionDialog dialog;
    EXPECT_EQ(dialog.connection().provider, Provider::Sqlite);
}

TEST(ConnectionDialogTest, ReadsASqliteFile) {
    const ConnectionDialog dialog;
    field<QLineEdit>(dialog, "fileEdit")->setText(" /tmp/app.db ");

    const auto info = dialog.connection();
    EXPECT_TRUE(info.isFile());
    EXPECT_EQ(info.filePath, "/tmp/app.db");
}

TEST(ConnectionDialogTest, ChoosingAProviderFillsItsDefaultPort) {
    const ConnectionDialog dialog;
    auto *port = field<QSpinBox>(dialog, "portSpin");

    selectProvider(dialog, Provider::PostgreSql);
    EXPECT_EQ(port->value(), 5432);
    selectProvider(dialog, Provider::MySql);
    EXPECT_EQ(port->value(), 3306);
    selectProvider(dialog, Provider::SqlServer);
    EXPECT_EQ(port->value(), 1433);
}

TEST(ConnectionDialogTest, ReadsAServerConnection) {
    const ConnectionDialog dialog;
    selectProvider(dialog, Provider::PostgreSql);
    field<QLineEdit>(dialog, "hostEdit")->setText("db.local");
    field<QSpinBox>(dialog, "portSpin")->setValue(6543);
    field<QLineEdit>(dialog, "databaseEdit")->setText("shop");
    field<QLineEdit>(dialog, "userEdit")->setText("alice");
    field<QLineEdit>(dialog, "passwordEdit")->setText("s3cret");

    const auto info = dialog.connection();
    EXPECT_EQ(info.provider, Provider::PostgreSql);
    EXPECT_EQ(info.host, "db.local");
    EXPECT_EQ(info.port, 6543);
    EXPECT_EQ(info.databaseName, "shop");
    EXPECT_EQ(info.user, "alice");
    EXPECT_EQ(info.password, "s3cret");
}

TEST(ConnectionDialogTest, LeavesTheDefaultPortImplicit) {
    const ConnectionDialog dialog;
    selectProvider(dialog, Provider::MySql);
    EXPECT_EQ(dialog.connection().port, 0);
}

TEST(ConnectionDialogTest, IntegratedAuthDropsUserAndPassword) {
    const ConnectionDialog dialog;
    selectProvider(dialog, Provider::SqlServer);
    field<QLineEdit>(dialog, "userEdit")->setText("sa");
    field<QLineEdit>(dialog, "passwordEdit")->setText("pw");
    field<QCheckBox>(dialog, "integratedAuthCheck")->setChecked(true);

    const auto info = dialog.connection();
    EXPECT_TRUE(info.integratedAuth);
    EXPECT_TRUE(info.user.isEmpty());
    EXPECT_TRUE(info.password.isEmpty());
    EXPECT_FALSE(field<QLineEdit>(dialog, "passwordEdit")->isEnabled());
}

TEST(ConnectionDialogTest, SetConnectionRoundTrips) {
    ConnectionDialog dialog;
    auto original = ConnectionInfo::fromUrl("mssql://sa@sql1:14330/Sales?trust=1&driver=ODBC%20Driver%2017%20for%20SQL%20Server");
    original.password = "pw";
    dialog.setConnection(original);

    const auto back = dialog.connection();
    EXPECT_EQ(back.toUrl(), original.toUrl());
    EXPECT_EQ(back.password, "pw");
}

TEST(ConnectionDialogTest, TestReportsTheTestersVerdict) {
    ConnectionInfo seen;
    ConnectionDialog dialog(nullptr, [&seen](const ConnectionInfo &info) {
        seen = info;
        return QString("password authentication failed");
    });
    selectProvider(dialog, Provider::PostgreSql);
    field<QLineEdit>(dialog, "hostEdit")->setText("db");

    EXPECT_EQ(dialog.testConnection(), "password authentication failed");
    EXPECT_EQ(seen.host, "db");
    EXPECT_EQ(field<QLabel>(dialog, "statusLabel")->text(), "password authentication failed");
}

TEST(ConnectionDialogTest, TestReportsSuccess) {
    ConnectionDialog dialog(nullptr, [](const ConnectionInfo &) { return QString(); });

    EXPECT_TRUE(dialog.testConnection().isEmpty());
    EXPECT_EQ(field<QLabel>(dialog, "statusLabel")->text(), "Connection succeeded.");
}
