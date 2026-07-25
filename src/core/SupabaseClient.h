#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>
#include <functional>

class Session;

// Thin wrapper around the Supabase REST surface: password auth, Postgrest
// (the `/rest/v1/...` tables) and Edge Functions (`/functions/v1/...`).
class SupabaseClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(bool ok, const QJsonDocument &body, const QString &error)>;

    explicit SupabaseClient(Session &session, QObject *parent = nullptr);

    void signIn(const QString &email, const QString &password,
                std::function<void(bool ok, const QString &error)> callback);

    void invokeFunction(const QString &functionName, const QJsonObject &body, JsonCallback callback);

    void restGet(const QString &path, JsonCallback callback);
    void restPost(const QString &path, const QJsonDocument &body, JsonCallback callback);
    void restPatch(const QString &path, const QJsonDocument &body, JsonCallback callback);
    void restDelete(const QString &path, JsonCallback callback);

signals:
    // Emitted when the refresh token itself is rejected (expired/revoked), so
    // the stored session had to be cleared. The UI should prompt for login again.
    void sessionExpired();

private:
    void send(const QString &method, const QUrl &url, const QByteArray &body, JsonCallback callback,
              bool isRetryAfterRefresh = false);
    void refreshSession(std::function<void(bool ok)> callback);
    QNetworkRequest buildRequest(const QUrl &url) const;

    Session &m_session;
    QNetworkAccessManager m_networkManager;
    bool m_refreshInFlight = false;
    QVector<std::function<void(bool)>> m_pendingRefreshCallbacks;
};
