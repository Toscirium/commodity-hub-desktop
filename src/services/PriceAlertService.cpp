#include "PriceAlertService.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>

#include "core/Session.h"
#include "core/SupabaseClient.h"

PriceAlertService::PriceAlertService(SupabaseClient &client, Session &session, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_session(session)
{
    // Alert evaluation happens server-side (evaluate-price-alerts edge
    // function); the client just polls for newly-fired, undismissed triggers,
    // matching the web app's 60s refetch interval.
    m_triggerPollTimer.setInterval(60'000);
    connect(&m_triggerPollTimer, &QTimer::timeout, this, &PriceAlertService::fetchUndismissedTriggers);
    m_triggerPollTimer.start();
}

void PriceAlertService::fetchAlerts()
{
    QPointer<PriceAlertService> self(this);
    m_client.restGet(QStringLiteral("/rest/v1/price_alerts?select=*&order=created_at.desc"),
                      [self](bool ok, const QJsonDocument &doc, const QString &error) {
                          if (!self)
                              return;

                          if (!ok) {
                              emit self->errorOccurred(error);
                              return;
                          }

                          QVector<PriceAlert> alerts;
                          const QJsonArray array = doc.array();
                          alerts.reserve(array.size());
                          for (const QJsonValue &value : array)
                              alerts.append(PriceAlert::fromJson(value.toObject()));

                          emit self->alertsLoaded(alerts);
                      });
}

void PriceAlertService::createAlert(const QString &commodityName, const QString &commoditySymbol,
                                     const QString &condition, double targetPrice, const QString &note)
{
    QJsonObject body;
    body["commodity_name"] = commodityName;
    body["commodity_symbol"] = commoditySymbol;
    body["alert_type"] = "price";
    body["condition"] = condition;
    body["target_price"] = targetPrice;
    body["user_id"] = m_session.userId();
    if (!note.isEmpty())
        body["note"] = note;

    QPointer<PriceAlertService> self(this);
    m_client.restPost(QStringLiteral("/rest/v1/price_alerts"), QJsonDocument(body),
                       [self](bool ok, const QJsonDocument &, const QString &error) {
                           if (!self)
                               return;
                           if (!ok) {
                               emit self->errorOccurred(error);
                               return;
                           }
                           self->fetchAlerts();
                       });
}

void PriceAlertService::setAlertActive(const QString &alertId, bool active)
{
    QJsonObject body;
    body["is_active"] = active;

    const QString path = QStringLiteral("/rest/v1/price_alerts?id=eq.") + QUrl::toPercentEncoding(alertId);

    QPointer<PriceAlertService> self(this);
    m_client.restPatch(path, QJsonDocument(body), [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        self->fetchAlerts();
    });
}

void PriceAlertService::deleteAlert(const QString &alertId)
{
    const QString path = QStringLiteral("/rest/v1/price_alerts?id=eq.") + QUrl::toPercentEncoding(alertId);

    QPointer<PriceAlertService> self(this);
    m_client.restDelete(path, [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        self->fetchAlerts();
    });
}

void PriceAlertService::fetchUndismissedTriggers()
{
    QPointer<PriceAlertService> self(this);
    m_client.restGet(
        QStringLiteral("/rest/v1/price_alert_triggers?select=*&dismissed_at=is.null&order=triggered_at.desc&limit=20"),
        [self](bool ok, const QJsonDocument &doc, const QString &error) {
            if (!self)
                return;

            if (!ok) {
                emit self->errorOccurred(error);
                return;
            }

            QVector<PriceAlertTrigger> triggers;
            const QJsonArray array = doc.array();
            triggers.reserve(array.size());
            for (const QJsonValue &value : array)
                triggers.append(PriceAlertTrigger::fromJson(value.toObject()));

            emit self->triggersLoaded(triggers);
        });
}

void PriceAlertService::dismissTrigger(const QString &triggerId)
{
    QJsonObject body;
    body["dismissed_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    const QString path = QStringLiteral("/rest/v1/price_alert_triggers?id=eq.") + QUrl::toPercentEncoding(triggerId);

    QPointer<PriceAlertService> self(this);
    m_client.restPatch(path, QJsonDocument(body), [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        self->fetchUndismissedTriggers();
    });
}
