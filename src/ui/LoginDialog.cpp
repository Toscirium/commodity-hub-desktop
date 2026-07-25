#include "LoginDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/Config.h"
#include "core/SupabaseClient.h"

LoginDialog::LoginDialog(SupabaseClient &client, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
{
    setWindowTitle(Config::ApplicationName);
    setMinimumWidth(340);

    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText(tr("Email"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(tr("Password"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setWordWrap(true);

    m_loginButton = new QPushButton(tr("Log In"), this);
    m_loginButton->setDefault(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Email"), m_emailEdit);
    form->addRow(tr("Password"), m_passwordEdit);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Sign in to CommodityHub")));
    layout->addLayout(form);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_loginButton);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::onLoginClicked()
{
    const QString email = m_emailEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(tr("Enter both email and password."));
        return;
    }

    m_loginButton->setEnabled(false);
    m_statusLabel->setText(tr("Signing in..."));

    QPointer<LoginDialog> self(this);
    m_client.signIn(email, password, [self](bool ok, const QString &error) {
        if (!self)
            return;

        if (ok) {
            self->accept();
            return;
        }

        self->m_statusLabel->setText(error);
        self->m_loginButton->setEnabled(true);
    });
}
