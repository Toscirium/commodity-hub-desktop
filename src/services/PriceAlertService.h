#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>

#include "models/PriceAlert.h"

class SupabaseClient;
class Session;

class PriceAlertService : public QObject
{
    Q_OBJECT
public:
    PriceAlertService(SupabaseClient &client, Session &session, QObject *parent = nullptr);

    void fetchAlerts();
    void createAlert(const QString &commodityName, const QString &commoditySymbol, const QString &condition,
                      double targetPrice, const QString &note);
    void setAlertActive(const QString &alertId, bool active);
    void deleteAlert(const QString &alertId);

    void fetchUndismissedTriggers();
    void dismissTrigger(const QString &triggerId);

signals:
    void alertsLoaded(const QVector<PriceAlert> &alerts);
    void triggersLoaded(const QVector<PriceAlertTrigger> &triggers);
    void errorOccurred(const QString &message);

private:
    SupabaseClient &m_client;
    Session &m_session;
    QTimer m_triggerPollTimer;
};
