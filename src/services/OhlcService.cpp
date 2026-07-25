#include "OhlcService.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>

#include "core/SupabaseClient.h"

OhlcService::OhlcService(SupabaseClient &client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

void OhlcService::fetchHistory(const QString &commodityName, const QString &timeframe)
{
    QJsonObject body;
    body["commodityName"] = commodityName;
    body["timeframe"] = timeframe;
    body["chartType"] = "line";

    QPointer<OhlcService> self(this);
    m_client.invokeFunction(QStringLiteral("fetch-commodity-data"), body,
                             [self, commodityName](bool ok, const QJsonDocument &doc, const QString &error) {
                                 if (!self)
                                     return;

                                 if (!ok) {
                                     emit self->errorOccurred(error);
                                     return;
                                 }

                                 QVector<OhlcBar> bars;
                                 const QJsonArray array = doc.object().value("data").toArray();
                                 bars.reserve(array.size());
                                 for (const QJsonValue &value : array)
                                     bars.append(OhlcBar::fromJson(value.toObject()));

                                 const bool ohlcAvailable = doc.object().value("ohlcAvailable").toBool();
                                 emit self->historyLoaded(commodityName, bars, ohlcAvailable);
                             });
}
