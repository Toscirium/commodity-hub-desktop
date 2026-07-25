#pragma once

#include <QDialog>

class SupabaseClient;
class QLineEdit;
class QLabel;
class QPushButton;

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(SupabaseClient &client, QWidget *parent = nullptr);

private slots:
    void onLoginClicked();

private:
    SupabaseClient &m_client;
    QLineEdit *m_emailEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_statusLabel;
    QPushButton *m_loginButton;
};
