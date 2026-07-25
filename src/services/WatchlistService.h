#pragma once

#include <QObject>
#include <QVector>

#include "models/Watchlist.h"

class SupabaseClient;
class Session;

class WatchlistService : public QObject
{
    Q_OBJECT
public:
    WatchlistService(SupabaseClient &client, Session &session, QObject *parent = nullptr);

    void fetchWatchlists();
    void createWatchlist(const QString &name);
    void addItem(const QString &watchlistId, const QString &commodityName, const QString &commoditySymbol);
    void removeItem(const QString &itemId);

signals:
    void watchlistsLoaded(const QVector<Watchlist> &watchlists);
    void errorOccurred(const QString &message);
    void itemsChanged();

private:
    void fetchItemsForWatchlists(QVector<Watchlist> watchlists);

    SupabaseClient &m_client;
    Session &m_session;
};
