#pragma once

#include <QDateTime>
#include <QString>

// Holds the current Supabase auth session and persists it to QSettings so the
// user isn't asked to log in again on every launch.
class Session
{
public:
    Session() = default;

    bool isValid() const;

    const QString &accessToken() const { return m_accessToken; }
    const QString &refreshToken() const { return m_refreshToken; }
    const QString &userId() const { return m_userId; }
    const QString &email() const { return m_email; }

    void setTokens(const QString &accessToken, const QString &refreshToken,
                    int expiresInSeconds, const QString &userId, const QString &email);
    void clear();

    void load();
    void save() const;

private:
    QString m_accessToken;
    QString m_refreshToken;
    QString m_userId;
    QString m_email;
    QDateTime m_expiresAt;
};
