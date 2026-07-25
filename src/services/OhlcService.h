#pragma once

#include <QObject>
#include <QVector>

#include "models/OhlcBar.h"

class SupabaseClient;

class OhlcService : public QObject
{
    Q_OBJECT
public:
    explicit OhlcService(SupabaseClient &client, QObject *parent = nullptr);

    // timeframe: one of "1d", "1m", "3m", "6m", "1y", "2y"
    void fetchHistory(const QString &commodityName, const QString &timeframe);

signals:
    // ohlcAvailable mirrors the API's flag: false for timeframes (e.g. "1d")
    // where no provider gives true OHLC bars, only a close price.
    void historyLoaded(const QString &commodityName, const QVector<OhlcBar> &bars, bool ohlcAvailable);
    void errorOccurred(const QString &message);

private:
    SupabaseClient &m_client;
};
