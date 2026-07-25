#include "PriceAlert.h"

PriceAlert PriceAlert::fromJson(const QJsonObject &json)
{
    PriceAlert alert;
    alert.id = json.value("id").toString();
    alert.commodityName = json.value("commodity_name").toString();
    alert.commoditySymbol = json.value("commodity_symbol").toString();
    alert.condition = json.value("condition").toString();
    alert.targetPrice = json.value("target_price").toDouble();
    alert.isActive = json.value("is_active").toBool();
    alert.lastTriggeredAt = json.value("last_triggered_at").toString();
    alert.note = json.value("note").toString();
    return alert;
}

PriceAlertTrigger PriceAlertTrigger::fromJson(const QJsonObject &json)
{
    PriceAlertTrigger trigger;
    trigger.id = json.value("id").toString();
    trigger.alertId = json.value("alert_id").toString();
    trigger.commodityName = json.value("commodity_name").toString();
    trigger.condition = json.value("condition").toString();
    trigger.targetPrice = json.value("target_price").toDouble();
    trigger.triggeredPrice = json.value("triggered_price").toDouble();
    trigger.triggeredAt = json.value("triggered_at").toString();
    return trigger;
}
