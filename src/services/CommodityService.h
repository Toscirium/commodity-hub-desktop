#pragma once

#include <QObject>
#include <QVector>

#include "models/Commodity.h"

class SupabaseClient;

class CommodityService : public QObject
{
    Q_OBJECT
public:
    explicit CommodityService(SupabaseClient &client, QObject *parent = nullptr);

    void fetchAll();

signals:
    void commoditiesLoaded(const QVector<Commodity> &commodities);
    void errorOccurred(const QString &message);

private:
    SupabaseClient &m_client;
};
