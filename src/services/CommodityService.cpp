#include "CommodityService.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>

#include "core/SupabaseClient.h"

CommodityService::CommodityService(SupabaseClient &client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

void CommodityService::fetchAll()
{
    QPointer<CommodityService> self(this);
    m_client.invokeFunction(QStringLiteral("fetch-all-commodities"), QJsonObject(),
                             [self](bool ok, const QJsonDocument &doc, const QString &error) {
                                 if (!self)
                                     return;

                                 if (!ok) {
                                     qDebug() << "[commodities] request failed:" << error << doc;
                                     emit self->errorOccurred(error);
                                     return;
                                 }

                                 QVector<Commodity> commodities;
                                 const QJsonArray array = doc.object().value("commodities").toArray();
                                 commodities.reserve(array.size());
                                 for (const QJsonValue &value : array)
                                     commodities.append(Commodity::fromJson(value.toObject()));

                                 qDebug() << "[commodities] loaded" << commodities.size() << "commodities";
                                 if (!commodities.isEmpty())
                                     qDebug() << "  first:" << commodities.first().name << commodities.first().price;

                                 emit self->commoditiesLoaded(commodities);
                             });
}
