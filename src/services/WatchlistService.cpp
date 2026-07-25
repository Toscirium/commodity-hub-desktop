#include "WatchlistService.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>

#include "core/Session.h"
#include "core/SupabaseClient.h"

WatchlistService::WatchlistService(SupabaseClient &client, Session &session, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_session(session)
{
}

void WatchlistService::fetchWatchlists()
{
    QPointer<WatchlistService> self(this);
    m_client.restGet(QStringLiteral("/rest/v1/watchlists?select=*&order=sort_order.asc"),
                      [self](bool ok, const QJsonDocument &doc, const QString &error) {
                          if (!self)
                              return;

                          if (!ok) {
                              qDebug() << "[watchlists] request failed:" << error << doc;
                              emit self->errorOccurred(error);
                              return;
                          }

                          QVector<Watchlist> watchlists;
                          const QJsonArray array = doc.array();
                          watchlists.reserve(array.size());
                          for (const QJsonValue &value : array)
                              watchlists.append(Watchlist::fromJson(value.toObject()));

                          qDebug() << "[watchlists] loaded" << watchlists.size() << "watchlist(s)";
                          for (const Watchlist &wl : watchlists)
                              qDebug() << "  -" << wl.id << wl.name << "default=" << wl.isDefault;

                          self->fetchItemsForWatchlists(watchlists);
                      });
}

void WatchlistService::fetchItemsForWatchlists(QVector<Watchlist> watchlists)
{
    QPointer<WatchlistService> self(this);
    m_client.restGet(QStringLiteral("/rest/v1/watchlist_items?select=*&order=position.asc"),
                      [self, watchlists](bool ok, const QJsonDocument &doc, const QString &error) mutable {
                          if (!self)
                              return;

                          if (!ok) {
                              qDebug() << "[watchlist_items] request failed:" << error << doc;
                              emit self->errorOccurred(error);
                              return;
                          }

                          const QJsonArray array = doc.array();
                          qDebug() << "[watchlist_items] loaded" << array.size() << "item(s) total";

                          int unmatched = 0;
                          for (const QJsonValue &value : array) {
                              const WatchlistItem item = WatchlistItem::fromJson(value.toObject());
                              bool matched = false;
                              for (Watchlist &list : watchlists) {
                                  if (list.id == item.watchlistId) {
                                      list.items.append(item);
                                      matched = true;
                                      break;
                                  }
                              }
                              if (!matched) {
                                  ++unmatched;
                                  qDebug() << "  ! item" << item.id << "commodity=" << item.commodityName
                                           << "references unknown watchlist_id=" << item.watchlistId;
                              }
                          }
                          if (unmatched > 0)
                              qDebug() << "[watchlist_items]" << unmatched << "item(s) did not match any fetched watchlist";

                          emit self->watchlistsLoaded(watchlists);
                      });
}

void WatchlistService::createWatchlist(const QString &name)
{
    QJsonObject body;
    body["name"] = name;
    body["user_id"] = m_session.userId();

    QPointer<WatchlistService> self(this);
    m_client.restPost(QStringLiteral("/rest/v1/watchlists"), QJsonDocument(body),
                       [self](bool ok, const QJsonDocument &doc, const QString &error) {
                           if (!self)
                               return;
                           if (!ok) {
                               qDebug() << "[watchlists] create failed:" << error << doc;
                               emit self->errorOccurred(error);
                               return;
                           }
                           emit self->itemsChanged();
                       });
}

void WatchlistService::addItem(const QString &watchlistId, const QString &commodityName,
                                const QString &commoditySymbol)
{
    QJsonObject body;
    body["watchlist_id"] = watchlistId;
    body["commodity_name"] = commodityName;
    body["commodity_symbol"] = commoditySymbol;
    body["user_id"] = m_session.userId();

    QPointer<WatchlistService> self(this);
    m_client.restPost(QStringLiteral("/rest/v1/watchlist_items"), QJsonDocument(body),
                       [self](bool ok, const QJsonDocument &, const QString &error) {
                           if (!self)
                               return;
                           if (!ok) {
                               emit self->errorOccurred(error);
                               return;
                           }
                           emit self->itemsChanged();
                       });
}

void WatchlistService::removeItem(const QString &itemId)
{
    const QString path = QStringLiteral("/rest/v1/watchlist_items?id=eq.") + QUrl::toPercentEncoding(itemId);

    QPointer<WatchlistService> self(this);
    m_client.restDelete(path, [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        emit self->itemsChanged();
    });
}
