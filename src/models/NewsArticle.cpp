#include "NewsArticle.h"

NewsArticle NewsArticle::fromJson(const QJsonObject &json)
{
    NewsArticle article;
    article.title = json.value("title").toString();
    article.description = json.value("description").toString();
    article.url = json.value("url").toString();
    article.source = json.value("source").toString();
    article.publishedAt = json.value("publishedAt").toString();
    article.category = json.value("category").toString();
    return article;
}
