#include "Session.h"

#include <QSettings>

#include "Config.h"

bool Session::isValid() const
{
    if (m_accessToken.isEmpty())
        return false;
    if (!m_expiresAt.isValid())
        return true;
    return QDateTime::currentDateTimeUtc() < m_expiresAt;
}

void Session::setTokens(const QString &accessToken, const QString &refreshToken,
                         int expiresInSeconds, const QString &userId, const QString &email)
{
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;
    m_userId = userId;
    m_email = email;
    m_expiresAt = QDateTime::currentDateTimeUtc().addSecs(expiresInSeconds);
}

void Session::clear()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_userId.clear();
    m_email.clear();
    m_expiresAt = QDateTime();

    QSettings settings(Config::OrganizationName, Config::ApplicationName);
    settings.beginGroup("session");
    settings.remove("");
    settings.endGroup();
}

void Session::load()
{
    QSettings settings(Config::OrganizationName, Config::ApplicationName);
    settings.beginGroup("session");
    m_accessToken = settings.value("accessToken").toString();
    m_refreshToken = settings.value("refreshToken").toString();
    m_userId = settings.value("userId").toString();
    m_email = settings.value("email").toString();
    m_expiresAt = settings.value("expiresAt").toDateTime();
    settings.endGroup();
}

void Session::save() const
{
    QSettings settings(Config::OrganizationName, Config::ApplicationName);
    settings.beginGroup("session");
    settings.setValue("accessToken", m_accessToken);
    settings.setValue("refreshToken", m_refreshToken);
    settings.setValue("userId", m_userId);
    settings.setValue("email", m_email);
    settings.setValue("expiresAt", m_expiresAt);
    settings.endGroup();
}
