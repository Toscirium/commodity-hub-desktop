#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

// Mirrors the `watchlist_items` table row in commodity-hub's Supabase schema.
struct WatchlistItem
{
    QString id;
    QString watchlistId;
    QString commodityName;
    QString commoditySymbol;
    int position = 0;

    static WatchlistItem fromJson(const QJsonObject &json);
};

// Mirrors the `watchlists` table row.
struct Watchlist
{
    QString id;
    QString name;
    QString color;
    bool isDefault = false;
    int sortOrder = 0;
    QVector<WatchlistItem> items;

    static Watchlist fromJson(const QJsonObject &json);
};
