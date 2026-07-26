#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors the article shape returned by commodity-hub's
// supabase/functions/enhanced-commodity-news edge function.
struct NewsArticle
{
    QString title;
    QString description;
    QString url;
    QString source;
    QString publishedAt; // ISO 8601
    QString category;

    static NewsArticle fromJson(const QJsonObject &json);
};
