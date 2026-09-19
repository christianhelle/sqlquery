#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <functional>

#include "../database/connectioninfo.h"

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;
class QStackedWidget;

// Asks where a Connection goes: a file for SQLite, or a server, database and
// credentials for the rest. Test tries the Connection without keeping it open.
class ConnectionDialog final : public QDialog {
    Q_OBJECT

public:
    // Opens the Connection and says why it failed; empty when it opened.
    using Tester = std::function<QString(const ConnectionInfo &)>;

    explicit ConnectionDialog(QWidget *parent = nullptr, Tester tester = {});

    // Fills the form from a Connection, as when reopening a recent one.
    void setConnection(const ConnectionInfo &connection);

    // The Connection the form describes, password included.
    [[nodiscard]] ConnectionInfo connection() const;

    // Runs the tester and shows the outcome. Returns the error, empty on success.
    QString testConnection();

private slots:
    void providerChanged();

    void browseForFile();

private:
    [[nodiscard]] Provider selectedProvider() const;

    void updateAuthFields() const;

    Tester tester;
    QComboBox *providerCombo;
    QStackedWidget *pages;
    QLineEdit *fileEdit;
    QFormLayout *serverForm;
    QLineEdit *hostEdit;
    QSpinBox *portSpin;
    QLineEdit *databaseEdit;
    QLineEdit *userEdit;
    QLineEdit *passwordEdit;
    QCheckBox *integratedAuthCheck;
    QLineEdit *odbcDriverEdit;
    QCheckBox *trustCertificateCheck;
    QLabel *statusLabel;
};

#endif // CONNECTIONDIALOG_H
