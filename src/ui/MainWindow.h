#pragma once

#include <QMainWindow>
#include <QSet>
#include <QVector>

#include "models/Commodity.h"
#include "models/PriceAlert.h"

class Session;
class SupabaseClient;
class CommodityService;
class OhlcService;
class NewsService;
class WatchlistService;
class PortfolioService;
class PriceAlertService;
class WatchlistPanel;
class DashboardPanel;
class PortfolioPanel;
class AlertsPanel;
class SpreadCalculatorPanel;
class ScreenerPanel;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QProgressBar;
class QLabel;
class QCloseEvent;
class QSystemTrayIcon;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(Session &session, SupabaseClient &client, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onCommoditySelected(const QString &commodityName);
    void onTimeframeChanged(const QString &timeframe);
    void refreshData();
    void onLogout();
    void onSessionExpired();
    void onNavRowChanged(int row);
    void onTriggersLoaded(const QVector<PriceAlertTrigger> &triggers);
    void quitFromTray();

private:
    // A sidebar row is either a non-selectable section label ("header"), a
    // Markets category (browses every commodity in that category), a Tools
    // destination mapping straight to a QStackedWidget page, or a "coming
    // soon" placeholder (Pro/Insights items with no feature behind them yet).
    struct NavRow
    {
        QString type; // "header" | "category" | "page" | "placeholder"
        QString category; // also used as the display label for "placeholder" rows
        int pageIndex = -1;
    };

    void setupUi();
    void setupMenu();
    void setupTrayIcon();
    void restoreWindowState();
    void rebuildNavList();
    QString currentNavSelectionKey() const;
    void selectNavByKey(const QString &key);
    void selectPage(int pageIndex);
    void selectCommodityAndShowChart(const QString &commodityName);
    void beginRequest();
    void endRequest();
    void showError(const QString &context, const QString &message);

    Session &m_session;
    SupabaseClient &m_client;
    CommodityService *m_commodityService;
    OhlcService *m_ohlcService;
    NewsService *m_newsService;
    WatchlistService *m_watchlistService;
    PortfolioService *m_portfolioService;
    PriceAlertService *m_priceAlertService;

    QListWidget *m_navList;
    QVector<NavRow> m_navRows;
    QStackedWidget *m_pages;
    DashboardPanel *m_dashboardPanel;
    WatchlistPanel *m_watchlistPanel;
    PortfolioPanel *m_portfolioPanel;
    AlertsPanel *m_alertsPanel;
    SpreadCalculatorPanel *m_spreadCalculatorPanel;
    ScreenerPanel *m_screenerPanel;

    QProgressBar *m_loadingIndicator;
    QLabel *m_errorBanner;

    QSystemTrayIcon *m_trayIcon = nullptr;
    bool m_quitting = false;
    bool m_trayHintShown = false;

    // Trigger ids already seen (across app polls), so the backlog of
    // already-fired alerts fetched right after login doesn't spam a
    // notification per row — only genuinely new ones do.
    QSet<QString> m_seenTriggerIds;
    bool m_triggersInitialized = false;

    QVector<Commodity> m_commodities;
    QString m_selectedCommodity;
    QString m_selectedTimeframe = QStringLiteral("3m");
    int m_pendingRequests = 0;
};
