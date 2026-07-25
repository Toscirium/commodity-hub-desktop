#pragma once

#include <QWidget>
#include <QVector>

#include "models/Commodity.h"
#include "models/PortfolioPosition.h"
#include "models/PriceAlert.h"
#include "models/Watchlist.h"

class QLabel;
class QListWidget;

// Read-only landing page: recomputes everything from data MainWindow already
// fetched for the other panels, so it needs no network calls of its own.
class DashboardPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);
    void setPositions(const QVector<PortfolioPosition> &positions);
    void setAlerts(const QVector<PriceAlert> &alerts);
    void setWatchlists(const QVector<Watchlist> &watchlists);

signals:
    void commoditySelected(const QString &commodityName);

private:
    void refreshStats();
    void refreshMovers();
    void populateMoverList(QListWidget *list, const QVector<Commodity> &movers);

    QVector<Commodity> m_commodities;
    QVector<PortfolioPosition> m_positions;
    QVector<PriceAlert> m_alerts;
    QVector<Watchlist> m_watchlists;

    QLabel *m_portfolioValueLabel;
    QLabel *m_portfolioReturnLabel;
    QLabel *m_activeAlertsLabel;
    QLabel *m_watchlistItemsLabel;
    QListWidget *m_gainersList;
    QListWidget *m_losersList;
};
