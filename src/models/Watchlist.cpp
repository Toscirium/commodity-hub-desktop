#include "Watchlist.h"

WatchlistItem WatchlistItem::fromJson(const QJsonObject &json)
{
    WatchlistItem item;
    item.id = json.value("id").toString();
    item.watchlistId = json.value("watchlist_id").toString();
    item.commodityName = json.value("commodity_name").toString();
    item.commoditySymbol = json.value("commodity_symbol").toString();
    item.position = json.value("position").toInt();
    return item;
}

Watchlist Watchlist::fromJson(const QJsonObject &json)
{
    Watchlist list;
    list.id = json.value("id").toString();
    list.name = json.value("name").toString();
    list.color = json.value("color").toString();
    list.isDefault = json.value("is_default").toBool();
    list.sortOrder = json.value("sort_order").toInt();
    return list;
}
