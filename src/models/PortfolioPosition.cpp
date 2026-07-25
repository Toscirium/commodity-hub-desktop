#include "PortfolioPosition.h"

PortfolioPosition PortfolioPosition::fromJson(const QJsonObject &json)
{
    PortfolioPosition position;
    position.id = json.value("id").toString();
    position.commodityName = json.value("commodity_name").toString();
    position.quantity = json.value("quantity").toDouble();
    position.entryPrice = json.value("entry_price").toDouble();
    position.entryDate = json.value("entry_date").toString();
    position.notes = json.value("notes").toString();
    return position;
}
