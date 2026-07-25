#include "DashboardPanel.h"

#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace {
const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);

QLabel *makeStatTile(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(QStringLiteral("card"));
    label->setAttribute(Qt::WA_StyledBackground, true);
    label->setTextFormat(Qt::RichText);
    label->setMargin(14);
    return label;
}

// Small uppercase label above a big monospace value, mirroring the price
// cards on commodity-hub.eu's dashboard/category pages.
QString statTileHtml(const QString &label, const QString &value, const QColor &valueColor)
{
    return QStringLiteral("<div style='font-size:11px;font-weight:600;letter-spacing:0.4px;color:#898d94;'>%1</div>"
                           "<div style='font-family:\"JetBrains Mono\";font-size:22px;font-weight:700;"
                           "color:%2;margin-top:6px;'>%3</div>")
        .arg(label.toUpper(), valueColor.name(), value);
}
}

DashboardPanel::DashboardPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *iconTile = new QLabel(QStringLiteral("\U0001F4CA"), this); // chart-bar, matches the sidebar's logo tile
    iconTile->setObjectName(QStringLiteral("logoTile"));
    iconTile->setAlignment(Qt::AlignCenter);
    iconTile->setFixedSize(36, 36);

    auto *eyebrow = new QLabel(tr("DASHBOARD"), this);
    eyebrow->setObjectName(QStringLiteral("panelHeading"));
    auto *heading = new QLabel(tr("Overview"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("Your markets, portfolio, and alerts at a glance"), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    auto *titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(2);
    titleColumn->addWidget(eyebrow);
    titleColumn->addWidget(heading);
    titleColumn->addWidget(subtitle);

    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(12);
    headerRow->addWidget(iconTile);
    headerRow->addLayout(titleColumn, 1);

    m_portfolioValueLabel = makeStatTile(this);
    m_portfolioReturnLabel = makeStatTile(this);
    m_activeAlertsLabel = makeStatTile(this);
    m_watchlistItemsLabel = makeStatTile(this);

    auto *statsGrid = new QGridLayout;
    statsGrid->addWidget(m_portfolioValueLabel, 0, 0);
    statsGrid->addWidget(m_portfolioReturnLabel, 0, 1);
    statsGrid->addWidget(m_activeAlertsLabel, 0, 2);
    statsGrid->addWidget(m_watchlistItemsLabel, 0, 3);

    auto *gainersHeading = new QLabel(tr("Top Gainers"), this);
    gainersHeading->setObjectName(QStringLiteral("panelHeading"));
    m_gainersList = new QListWidget(this);
    m_gainersList->setFrameShape(QFrame::NoFrame);

    auto *losersHeading = new QLabel(tr("Top Losers"), this);
    losersHeading->setObjectName(QStringLiteral("panelHeading"));
    m_losersList = new QListWidget(this);
    m_losersList->setFrameShape(QFrame::NoFrame);

    auto *gainersColumn = new QVBoxLayout;
    gainersColumn->addWidget(gainersHeading);
    gainersColumn->addWidget(m_gainersList, 1);

    auto *losersColumn = new QVBoxLayout;
    losersColumn->addWidget(losersHeading);
    losersColumn->addWidget(m_losersList, 1);

    auto *moversRow = new QHBoxLayout;
    moversRow->addLayout(gainersColumn);
    moversRow->addLayout(losersColumn);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);
    layout->addLayout(headerRow);
    layout->addLayout(statsGrid);
    layout->addLayout(moversRow, 1);

    connect(m_gainersList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit commoditySelected(item->data(Qt::UserRole).toString());
    });
    connect(m_losersList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit commoditySelected(item->data(Qt::UserRole).toString());
    });

    refreshStats();
    refreshMovers();
}

void DashboardPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;
    refreshStats();
    refreshMovers();
}

void DashboardPanel::setPositions(const QVector<PortfolioPosition> &positions)
{
    m_positions = positions;
    refreshStats();
}

void DashboardPanel::setAlerts(const QVector<PriceAlert> &alerts)
{
    m_alerts = alerts;
    refreshStats();
}

void DashboardPanel::setWatchlists(const QVector<Watchlist> &watchlists)
{
    m_watchlists = watchlists;
    refreshStats();
}

void DashboardPanel::refreshStats()
{
    double totalValue = 0.0;
    double totalCost = 0.0;
    for (const PortfolioPosition &position : m_positions) {
        totalCost += position.quantity * position.entryPrice;
        double currentPrice = position.entryPrice;
        for (const Commodity &c : m_commodities) {
            if (c.name == position.commodityName) {
                currentPrice = c.price;
                break;
            }
        }
        totalValue += position.quantity * currentPrice;
    }
    const double totalReturn = totalValue - totalCost;
    const double returnPct = totalCost > 0.0 ? (totalReturn / totalCost) * 100.0 : 0.0;

    const QColor foreground(229, 231, 235); // --foreground

    m_portfolioValueLabel->setText(
        statTileHtml(tr("Portfolio Value"), QString::number(totalValue, 'f', 2), foreground));

    const QString sign = totalReturn >= 0.0 ? "+" : "";
    const QColor returnColor = totalReturn >= 0.0 ? kPositiveColor : kNegativeColor;
    m_portfolioReturnLabel->setText(statTileHtml(
        tr("Portfolio Return"),
        tr("%1%2 (%3%4%)").arg(sign, QString::number(totalReturn, 'f', 2), sign, QString::number(returnPct, 'f', 2)),
        returnColor));

    int activeAlerts = 0;
    for (const PriceAlert &alert : m_alerts) {
        if (alert.isActive)
            ++activeAlerts;
    }
    m_activeAlertsLabel->setText(statTileHtml(tr("Active Alerts"), QString::number(activeAlerts), foreground));

    int watchlistItems = 0;
    for (const Watchlist &wl : m_watchlists)
        watchlistItems += wl.items.size();
    m_watchlistItemsLabel->setText(statTileHtml(tr("Watchlist Items"), QString::number(watchlistItems), foreground));
}

void DashboardPanel::refreshMovers()
{
    QVector<Commodity> sorted = m_commodities;
    std::sort(sorted.begin(), sorted.end(),
              [](const Commodity &a, const Commodity &b) { return a.changePercent > b.changePercent; });

    QVector<Commodity> gainers;
    for (int i = 0; i < sorted.size() && gainers.size() < 5; ++i) {
        if (sorted.at(i).changePercent > 0.0)
            gainers.append(sorted.at(i));
    }

    QVector<Commodity> losers;
    for (int i = sorted.size() - 1; i >= 0 && losers.size() < 5; --i) {
        if (sorted.at(i).changePercent < 0.0)
            losers.append(sorted.at(i));
    }

    populateMoverList(m_gainersList, gainers);
    populateMoverList(m_losersList, losers);
}

void DashboardPanel::populateMoverList(QListWidget *list, const QVector<Commodity> &movers)
{
    list->clear();

    if (movers.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No data yet"), list);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setForeground(QColor(137, 141, 148)); // --muted-foreground
        list->addItem(item);
        return;
    }

    for (const Commodity &c : movers) {
        auto *item = new QListWidgetItem(
            tr("%1  %2  %3%4%")
                .arg(c.name, QString::number(c.price, 'f', 2), c.changePercent >= 0.0 ? "+" : "",
                     QString::number(c.changePercent, 'f', 2)),
            list);
        item->setForeground(c.changePercent >= 0.0 ? kPositiveColor : kNegativeColor);
        item->setData(Qt::UserRole, c.name);
        list->addItem(item);
    }
}
