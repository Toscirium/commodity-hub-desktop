#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors the `portfolio_positions` table row in commodity-hub's Supabase schema.
struct PortfolioPosition
{
    QString id;
    QString commodityName;
    double quantity = 0.0;
    double entryPrice = 0.0;
    QString entryDate;
    QString notes;

    static PortfolioPosition fromJson(const QJsonObject &json);
};
