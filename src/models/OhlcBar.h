#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors ChartDataPoint from commodity-hub's supabase/functions/_shared/commodity-service.ts
struct OhlcBar
{
    QString date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;

    static OhlcBar fromJson(const QJsonObject &json);
};
