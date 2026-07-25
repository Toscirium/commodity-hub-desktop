#include "SupabaseClient.h"

#include <QJsonArray>
#include <QNetworkReply>
#include <QPointer>
#include <QUrl>

#include "Config.h"
#include "Session.h"

namespace {

QString extractErrorMessage(const QJsonDocument &doc, const QString &networkError)
{
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        for (const char *key : {"error_description", "msg", "message", "error"}) {
            const QJsonValue value = obj.value(key);
            if (value.isString() && !value.toString().isEmpty())
                return value.toString();
        }
    }
    return networkError.isEmpty() ? QStringLiteral("Request failed") : networkError;
}

} // namespace

SupabaseClient::SupabaseClient(Session &session, QObject *parent)
    : QObject(parent)
    , m_session(session)
{
}

QNetworkRequest SupabaseClient::buildRequest(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("apikey", Config::SupabaseAnonKey);

    const QString bearer = m_session.isValid() ? m_session.accessToken()
                                                 : QString::fromLatin1(Config::SupabaseAnonKey);
    request.setRawHeader("Authorization", ("Bearer " + bearer).toUtf8());
    return request;
}

void SupabaseClient::send(const QString &method, const QUrl &url, const QByteArray &body, JsonCallback callback,
                           bool isRetryAfterRefresh)
{
    QNetworkRequest request = buildRequest(url);
    if (method == QStringLiteral("POST") || method == QStringLiteral("PATCH"))
        request.setRawHeader("Prefer", "return=representation");

    QNetworkReply *reply = nullptr;
    if (method == QStringLiteral("GET")) {
        reply = m_networkManager.get(request);
    } else if (method == QStringLiteral("POST")) {
        reply = m_networkManager.post(request, body);
    } else {
        reply = m_networkManager.sendCustomRequest(request, method.toUtf8(), body);
    }

    QPointer<SupabaseClient> self(this);
    connect(reply, &QNetworkReply::finished, this,
            [self, reply, callback, method, url, body, isRetryAfterRefresh]() {
        reply->deleteLater();

        const QByteArray raw = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(raw);

        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError && httpStatus < 400;

        if (!self)
            return;

        if (ok) {
            if (callback)
                callback(true, doc, QString());
            return;
        }

        const QString error = extractErrorMessage(doc, reply->errorString());

        // A 401 mid-session means the access token expired; transparently refresh
        // and retry once before surfacing the failure to the caller.
        if (httpStatus == 401 && !isRetryAfterRefresh && !self->m_session.refreshToken().isEmpty()) {
            self->m_pendingRefreshCallbacks.append([self, method, url, body, callback, doc, error](bool refreshOk) {
                if (!self)
                    return;
                if (refreshOk) {
                    self->send(method, url, body, callback, true);
                } else if (callback) {
                    callback(false, doc, error);
                }
            });

            if (!self->m_refreshInFlight) {
                self->m_refreshInFlight = true;
                self->refreshSession([self](bool refreshOk) {
                    if (!self)
                        return;
                    self->m_refreshInFlight = false;
                    if (!refreshOk)
                        emit self->sessionExpired();

                    const auto pending = self->m_pendingRefreshCallbacks;
                    self->m_pendingRefreshCallbacks.clear();
                    for (const auto &pendingCallback : pending)
                        pendingCallback(refreshOk);
                });
            }
            return;
        }

        if (callback)
            callback(false, doc, error);
    });
}

void SupabaseClient::refreshSession(std::function<void(bool ok)> callback)
{
    const QString refreshToken = m_session.refreshToken();
    if (refreshToken.isEmpty()) {
        if (callback)
            callback(false);
        return;
    }

    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + "/auth/v1/token?grant_type=refresh_token");

    QJsonObject body;
    body["refresh_token"] = refreshToken;

    QPointer<SupabaseClient> self(this);
    send(QStringLiteral("POST"), url, QJsonDocument(body).toJson(QJsonDocument::Compact),
         [self, callback](bool ok, const QJsonDocument &doc, const QString &) {
             if (!self)
                 return;

             if (!ok) {
                 self->m_session.clear();
                 if (callback)
                     callback(false);
                 return;
             }

             const QJsonObject obj = doc.object();
             const QJsonObject user = obj.value("user").toObject();
             const QString userId = user.value("id").toString();
             const QString email = user.value("email").toString();
             self->m_session.setTokens(obj.value("access_token").toString(),
                                        obj.value("refresh_token").toString(),
                                        obj.value("expires_in").toInt(3600),
                                        userId.isEmpty() ? self->m_session.userId() : userId,
                                        email.isEmpty() ? self->m_session.email() : email);
             self->m_session.save();

             if (callback)
                 callback(true);
         },
         /*isRetryAfterRefresh=*/true);
}

void SupabaseClient::signIn(const QString &email, const QString &password,
                             std::function<void(bool ok, const QString &error)> callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + "/auth/v1/token?grant_type=password");

    QJsonObject body;
    body["email"] = email;
    body["password"] = password;

    send(QStringLiteral("POST"), url, QJsonDocument(body).toJson(QJsonDocument::Compact),
         [this, callback](bool ok, const QJsonDocument &doc, const QString &error) {
             if (!ok) {
                 if (callback)
                     callback(false, error);
                 return;
             }

             const QJsonObject obj = doc.object();
             const QJsonObject user = obj.value("user").toObject();
             m_session.setTokens(obj.value("access_token").toString(),
                                  obj.value("refresh_token").toString(),
                                  obj.value("expires_in").toInt(3600),
                                  user.value("id").toString(),
                                  user.value("email").toString());
             m_session.save();

             if (callback)
                 callback(true, QString());
         });
}

void SupabaseClient::invokeFunction(const QString &functionName, const QJsonObject &body, JsonCallback callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + "/functions/v1/" + functionName);
    send(QStringLiteral("POST"), url, QJsonDocument(body).toJson(QJsonDocument::Compact), std::move(callback));
}

void SupabaseClient::restGet(const QString &path, JsonCallback callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + path);
    send(QStringLiteral("GET"), url, QByteArray(), std::move(callback));
}

void SupabaseClient::restPost(const QString &path, const QJsonDocument &body, JsonCallback callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + path);
    send(QStringLiteral("POST"), url, body.toJson(QJsonDocument::Compact), std::move(callback));
}

void SupabaseClient::restPatch(const QString &path, const QJsonDocument &body, JsonCallback callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + path);
    send(QStringLiteral("PATCH"), url, body.toJson(QJsonDocument::Compact), std::move(callback));
}

void SupabaseClient::restDelete(const QString &path, JsonCallback callback)
{
    const QUrl url(QString::fromLatin1(Config::SupabaseUrl) + path);
    send(QStringLiteral("DELETE"), url, QByteArray(), std::move(callback));
}
