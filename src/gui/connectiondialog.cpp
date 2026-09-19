#include "connectiondialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "../database/providerdatabase.h"

namespace {
    constexpr int FilePage = 0;
    constexpr int ServerPage = 1;
}

ConnectionDialog::ConnectionDialog(QWidget *parent, Tester tester) :
    QDialog(parent),
    tester(tester ? std::move(tester) : Tester(::testConnection)) {
    setWindowTitle("Connect to Database");

    providerCombo = new QComboBox(this);
    providerCombo->setObjectName("providerCombo");
    for (const Provider provider: {Provider::Sqlite, Provider::PostgreSql, Provider::SqlServer, Provider::MySql})
        providerCombo->addItem(providerName(provider), static_cast<int>(provider));

    // SQLite: a file.
    auto *filePage = new QWidget(this);
    fileEdit = new QLineEdit(filePage);
    fileEdit->setObjectName("fileEdit");
    auto *browseButton = new QPushButton("Browse...", filePage);
    auto *fileRow = new QHBoxLayout();
    fileRow->addWidget(fileEdit);
    fileRow->addWidget(browseButton);
    auto *fileForm = new QFormLayout(filePage);
    fileForm->addRow("File:", fileRow);

    // Every other Provider: a server.
    auto *serverPage = new QWidget(this);
    hostEdit = new QLineEdit("localhost", serverPage);
    hostEdit->setObjectName("hostEdit");
    portSpin = new QSpinBox(serverPage);
    portSpin->setObjectName("portSpin");
    portSpin->setRange(1, 65535);
    databaseEdit = new QLineEdit(serverPage);
    databaseEdit->setObjectName("databaseEdit");
    userEdit = new QLineEdit(serverPage);
    userEdit->setObjectName("userEdit");
    passwordEdit = new QLineEdit(serverPage);
    passwordEdit->setObjectName("passwordEdit");
    passwordEdit->setEchoMode(QLineEdit::Password);
    integratedAuthCheck = new QCheckBox("Use Windows authentication", serverPage);
    integratedAuthCheck->setObjectName("integratedAuthCheck");
    odbcDriverEdit = new QLineEdit(ConnectionInfo::defaultOdbcDriver(), serverPage);
    odbcDriverEdit->setObjectName("odbcDriverEdit");
    trustCertificateCheck = new QCheckBox("Trust server certificate", serverPage);
    trustCertificateCheck->setObjectName("trustCertificateCheck");

    serverForm = new QFormLayout(serverPage);
    serverForm->addRow("Host:", hostEdit);
    serverForm->addRow("Port:", portSpin);
    serverForm->addRow("Database:", databaseEdit);
    serverForm->addRow(QString(), integratedAuthCheck);
    serverForm->addRow("User:", userEdit);
    serverForm->addRow("Password:", passwordEdit);
    serverForm->addRow("ODBC driver:", odbcDriverEdit);
    serverForm->addRow(QString(), trustCertificateCheck);

    pages = new QStackedWidget(this);
    pages->insertWidget(FilePage, filePage);
    pages->insertWidget(ServerPage, serverPage);

    statusLabel = new QLabel(this);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setWordWrap(true);
    statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto *testButton = buttons->addButton("Test Connection", QDialogButtonBox::ActionRole);

    auto *providerForm = new QFormLayout();
    providerForm->addRow("Provider:", providerCombo);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(providerForm);
    layout->addWidget(pages);
    layout->addWidget(statusLabel);
    layout->addWidget(buttons);

    connect(providerCombo, &QComboBox::currentIndexChanged, this, &ConnectionDialog::providerChanged);
    connect(integratedAuthCheck, &QCheckBox::toggled, this, [this] { updateAuthFields(); });
    connect(browseButton, &QPushButton::clicked, this, &ConnectionDialog::browseForFile);
    connect(testButton, &QPushButton::clicked, this, [this] { testConnection(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    providerChanged();
    resize(460, sizeHint().height());
}

Provider ConnectionDialog::selectedProvider() const {
    return static_cast<Provider>(providerCombo->currentData().toInt());
}

void ConnectionDialog::providerChanged() {
    const Provider provider = selectedProvider();
    const bool isFile = provider == Provider::Sqlite;
    pages->setCurrentIndex(isFile ? FilePage : ServerPage);
    if (!isFile)
        portSpin->setValue(defaultPort(provider));

    const bool isSqlServer = provider == Provider::SqlServer;
    serverForm->setRowVisible(integratedAuthCheck, isSqlServer);
    serverForm->setRowVisible(odbcDriverEdit, isSqlServer);
    serverForm->setRowVisible(trustCertificateCheck, isSqlServer);
    updateAuthFields();
    statusLabel->clear();
}

void ConnectionDialog::updateAuthFields() const {
    const bool integrated = selectedProvider() == Provider::SqlServer && integratedAuthCheck->isChecked();
    userEdit->setEnabled(!integrated);
    passwordEdit->setEnabled(!integrated);
}

void ConnectionDialog::browseForFile() {
    // Save-mode without the overwrite prompt, so both an existing file and a
    // new one can be picked: SQLite creates the file on open.
    const QString path = QFileDialog::getSaveFileName(this, "SQLite database", fileEdit->text(),
                                                      "SQLite databases (*.db *.sqlite *.sqlite3);;All files (*)",
                                                      nullptr, QFileDialog::DontConfirmOverwrite);
    if (!path.isEmpty())
        fileEdit->setText(path);
}

void ConnectionDialog::setConnection(const ConnectionInfo &connection) {
    providerCombo->setCurrentIndex(providerCombo->findData(static_cast<int>(connection.provider)));
    fileEdit->setText(connection.filePath);
    if (!connection.isFile()) {
        hostEdit->setText(connection.host);
        portSpin->setValue(connection.effectivePort());
    }
    databaseEdit->setText(connection.databaseName);
    userEdit->setText(connection.user);
    passwordEdit->setText(connection.password);
    integratedAuthCheck->setChecked(connection.integratedAuth);
    odbcDriverEdit->setText(connection.odbcDriver);
    trustCertificateCheck->setChecked(connection.trustServerCertificate);
    updateAuthFields();

    // A reopened server Connection comes back without its password, so that
    // is where the user has to type next.
    if (!connection.isFile() && !connection.integratedAuth && connection.password.isEmpty())
        passwordEdit->setFocus();
}

ConnectionInfo ConnectionDialog::connection() const {
    ConnectionInfo info;
    info.provider = selectedProvider();
    if (info.isFile()) {
        info.filePath = fileEdit->text().trimmed();
        return info;
    }

    info.host = hostEdit->text().trimmed();
    // The default port is left implicit, so the URL stays short.
    info.port = portSpin->value() == defaultPort(info.provider) ? 0 : portSpin->value();
    info.databaseName = databaseEdit->text().trimmed();
    if (info.provider == Provider::SqlServer) {
        info.integratedAuth = integratedAuthCheck->isChecked();
        info.odbcDriver = odbcDriverEdit->text().trimmed();
        info.trustServerCertificate = trustCertificateCheck->isChecked();
    }
    if (!info.integratedAuth) {
        info.user = userEdit->text().trimmed();
        info.password = passwordEdit->text();
    }
    return info;
}

QString ConnectionDialog::testConnection() {
    statusLabel->setText("Connecting...");
    statusLabel->repaint();
    const QString error = tester(connection());
    statusLabel->setText(error.isEmpty() ? "Connection succeeded." : error);
    return error;
}
