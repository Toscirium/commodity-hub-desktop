#include "NewsService.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>

#include "core/SupabaseClient.h"

NewsService::NewsService(SupabaseClient &client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

void NewsService::fetchNews(const QString &commodityName)
{
    QJsonObject body;
    body["commodity"] = commodityName;
    body["source"] = "all";

    QPointer<NewsService> self(this);
    m_client.invokeFunction(QStringLiteral("enhanced-commodity-news"), body,
                             [self, commodityName](bool ok, const QJsonDocument &doc, const QString &error) {
                                 if (!self)
                                     return;

                                 if (!ok) {
                                     emit self->errorOccurred(error);
                                     return;
                                 }

                                 QVector<NewsArticle> articles;
                                 const QJsonArray array = doc.object().value("articles").toArray();
                                 articles.reserve(array.size());
                                 for (const QJsonValue &value : array)
                                     articles.append(NewsArticle::fromJson(value.toObject()));

                                 emit self->newsLoaded(commodityName, articles);
                             });
}
