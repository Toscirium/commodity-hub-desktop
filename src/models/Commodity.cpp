#include "Commodity.h"

Commodity Commodity::fromJson(const QJsonObject &json)
{
    Commodity c;
    c.symbol = json.value("symbol").toString();
    c.name = json.value("name").toString();
    c.price = json.value("price").toDouble();
    c.change = json.value("change").toDouble();
    c.changePercent = json.value("changePercent").toDouble();
    c.volume = json.value("volume").toDouble();
    c.category = json.value("category").toString();
    c.contractSize = json.value("contractSize").toString();
    c.venue = json.value("venue").toString();
    c.isSynthetic = json.value("isSynthetic").toBool();
    return c;
}
