#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors the `price_alerts` table row. The web app also supports alert_type
// values beyond simple price thresholds (pct_move, volatility_band, ...); the
// desktop app only creates/manages the plain "price" type for now.
struct PriceAlert
{
    QString id;
    QString commodityName;
    QString commoditySymbol;
    QString condition; // "above" | "below"
    double targetPrice = 0.0;
    bool isActive = true;
    QString lastTriggeredAt;
    QString note;

    static PriceAlert fromJson(const QJsonObject &json);
};

// Mirrors the `price_alert_triggers` table row.
struct PriceAlertTrigger
{
    QString id;
    QString alertId;
    QString commodityName;
    QString condition;
    double targetPrice = 0.0;
    double triggeredPrice = 0.0;
    QString triggeredAt;

    static PriceAlertTrigger fromJson(const QJsonObject &json);
};
