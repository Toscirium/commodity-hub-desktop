#pragma once

#include <QObject>
#include <QVector>

#include "models/NewsArticle.h"

class SupabaseClient;

class NewsService : public QObject
{
    Q_OBJECT
public:
    explicit NewsService(SupabaseClient &client, QObject *parent = nullptr);

    void fetchNews(const QString &commodityName);

signals:
    void newsLoaded(const QString &commodityName, const QVector<NewsArticle> &articles);
    void errorOccurred(const QString &message);

private:
    SupabaseClient &m_client;
};
