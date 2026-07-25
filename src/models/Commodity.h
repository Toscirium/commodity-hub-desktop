#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors CommodityData from commodity-hub's supabase/functions/_shared/commodity-service.ts
struct Commodity
{
    QString symbol;
    QString name;
    double price = 0.0;
    double change = 0.0;
    double changePercent = 0.0;
    double volume = 0.0;
    QString category;
    QString contractSize;
    QString venue;
    bool isSynthetic = false;

    static Commodity fromJson(const QJsonObject &json);
};
