#include "Portfolio.h"

Portfolio Portfolio::fromJson(const QJsonObject &json)
{
    Portfolio portfolio;
    portfolio.id = json.value("id").toString();
    portfolio.name = json.value("name").toString();
    portfolio.isDefault = json.value("is_default").toBool();
    return portfolio;
}
