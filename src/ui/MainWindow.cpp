#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPair>
#include <QProgressBar>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

#include "AlertsPanel.h"
#include "ChartPanel.h"
#include "DashboardPanel.h"
#include "LoginDialog.h"
#include "NewsPanel.h"
#include "PortfolioPanel.h"
#include "WatchlistPanel.h"
#include "core/Config.h"
#include "core/Session.h"
#include "core/SupabaseClient.h"
#include "models/NewsArticle.h"
#include "models/OhlcBar.h"
#include "models/Portfolio.h"
#include "models/PortfolioPosition.h"
#include "models/PriceAlert.h"
#include "models/Watchlist.h"
#include "services/CommodityService.h"
#include "services/NewsService.h"
#include "services/OhlcService.h"
#include "services/PortfolioService.h"
#include "services/PriceAlertService.h"
#include "services/WatchlistService.h"

namespace {
constexpr int PageOverview = 0;
constexpr int PageWatchlist = 1;
constexpr int PagePortfolio = 2;
constexpr int PageAlerts = 3;

QSettings makeSettings()
{
    return QSettings(Config::OrganizationName, Config::ApplicationName);
}

QString capitalize(const QString &text)
{
    if (text.isEmpty())
        return text;
    return text.at(0).toUpper() + text.mid(1);
}

// Matches the order commodity-hub.eu lists Markets categories in; anything in
// the data but not in this list still shows up, just alphabetized afterward.
const QVector<QPair<QString, QString>> &canonicalCategories()
{
    static const QVector<QPair<QString, QString>> categories = {
        {QStringLiteral("energy"), QStringLiteral("⚡")},
        {QStringLiteral("metals"), QStringLiteral("◆")},
        {QStringLiteral("grains"), QStringLiteral("\U0001F33E")},
        {QStringLiteral("livestock"), QStringLiteral("\U0001F404")},
        {QStringLiteral("dairy"), QStringLiteral("\U0001F95B")},
        {QStringLiteral("industrials"), QStringLiteral("\U0001F3ED")},
        {QStringLiteral("emissions"), QStringLiteral("\U0001F343")},
    };
    return categories;
}
}

MainWindow::MainWindow(Session &session, SupabaseClient &client, QWidget *parent)
    : QMainWindow(parent)
    , m_session(session)
    , m_client(client)
{
    m_commodityService = new CommodityService(m_client, this);
    m_ohlcService = new OhlcService(m_client, this);
    m_newsService = new NewsService(m_client, this);
    m_watchlistService = new WatchlistService(m_client, m_session, this);
    m_portfolioService = new PortfolioService(m_client, m_session, this);
    m_priceAlertService = new PriceAlertService(m_client, m_session, this);

    setupUi();
    setupMenu();
    restoreWindowState();

    setWindowTitle(QStringLiteral("%1 — %2").arg(Config::ApplicationName, m_session.email()));

    connect(m_commodityService, &CommodityService::commoditiesLoaded, this,
            [this](const QVector<Commodity> &commodities) {
                endRequest();
                m_commodities = commodities;
                m_watchlistPanel->setCommodities(commodities);
                m_portfolioPanel->setCommodities(commodities);
                m_alertsPanel->setCommodities(commodities);
                m_dashboardPanel->setCommodities(commodities);
                rebuildNavList();
            });
    connect(m_commodityService, &CommodityService::errorOccurred, this, [this](const QString &msg) {
        endRequest();
        showError(tr("Commodities"), msg);
    });

    connect(m_watchlistService, &WatchlistService::watchlistsLoaded, this,
            [this](const QVector<Watchlist> &watchlists) {
                endRequest();
                m_watchlistPanel->setWatchlists(watchlists);
                m_dashboardPanel->setWatchlists(watchlists);
            });
    connect(m_watchlistService, &WatchlistService::errorOccurred, this, [this](const QString &msg) {
        endRequest();
        showError(tr("Watchlist"), msg);
    });
    connect(m_watchlistService, &WatchlistService::itemsChanged, m_watchlistService,
            &WatchlistService::fetchWatchlists);

    connect(m_ohlcService, &OhlcService::historyLoaded, this,
            [this](const QString &name, const QVector<OhlcBar> &bars, bool ohlcAvailable) {
                m_watchlistPanel->chartPanel()->setBars(name, bars, ohlcAvailable);
            });
    connect(m_ohlcService, &OhlcService::errorOccurred, this,
            [this](const QString &msg) { showError(tr("Chart"), msg); });

    connect(m_newsService, &NewsService::newsLoaded, this,
            [this](const QString &name, const QVector<NewsArticle> &articles) {
                m_watchlistPanel->newsPanel()->setArticles(name, articles);
            });
    connect(m_newsService, &NewsService::errorOccurred, this, [this](const QString &msg) {
        m_watchlistPanel->newsPanel()->setError(m_selectedCommodity, msg);
    });

    connect(m_portfolioService, &PortfolioService::portfoliosLoaded, this,
            [this](const QVector<Portfolio> &portfolios) {
                endRequest();
                m_portfolioPanel->setPortfolios(portfolios);
            });
    connect(m_portfolioService, &PortfolioService::positionsLoaded, this,
            [this](const QVector<PortfolioPosition> &positions) {
                m_portfolioPanel->setPositions(positions);
                m_dashboardPanel->setPositions(positions);
            });
    connect(m_portfolioService, &PortfolioService::errorOccurred, this, [this](const QString &msg) {
        endRequest();
        showError(tr("Portfolio"), msg);
    });

    connect(m_priceAlertService, &PriceAlertService::alertsLoaded, this,
            [this](const QVector<PriceAlert> &alerts) {
                endRequest();
                m_alertsPanel->setAlerts(alerts);
                m_dashboardPanel->setAlerts(alerts);
            });
    connect(m_priceAlertService, &PriceAlertService::triggersLoaded, m_alertsPanel, &AlertsPanel::setTriggers);
    connect(m_priceAlertService, &PriceAlertService::errorOccurred, this, [this](const QString &msg) {
        endRequest();
        showError(tr("Alerts"), msg);
    });

    connect(m_watchlistPanel, &WatchlistPanel::commoditySelected, this, &MainWindow::onCommoditySelected);
    connect(m_watchlistPanel, &WatchlistPanel::addItemRequested, m_watchlistService, &WatchlistService::addItem);
    connect(m_watchlistPanel, &WatchlistPanel::removeItemRequested, m_watchlistService, &WatchlistService::removeItem);
    connect(m_watchlistPanel, &WatchlistPanel::createWatchlistRequested, m_watchlistService,
            &WatchlistService::createWatchlist);

    connect(m_watchlistPanel->chartPanel(), &ChartPanel::timeframeChanged, this, &MainWindow::onTimeframeChanged);

    connect(m_dashboardPanel, &DashboardPanel::commoditySelected, this, &MainWindow::selectCommodityAndShowChart);

    connect(m_portfolioPanel, &PortfolioPanel::portfolioSelected, m_portfolioService,
            &PortfolioService::fetchPositions);
    connect(m_portfolioPanel, &PortfolioPanel::createPortfolioRequested, m_portfolioService,
            &PortfolioService::createPortfolio);
    connect(m_portfolioPanel, &PortfolioPanel::addPositionRequested, m_portfolioService,
            &PortfolioService::addPosition);
    connect(m_portfolioPanel, &PortfolioPanel::editPositionRequested, m_portfolioService,
            &PortfolioService::updatePosition);
    connect(m_portfolioPanel, &PortfolioPanel::removePositionRequested, m_portfolioService,
            &PortfolioService::removePosition);

    connect(m_alertsPanel, &AlertsPanel::createAlertRequested, m_priceAlertService, &PriceAlertService::createAlert);
    connect(m_alertsPanel, &AlertsPanel::deleteAlertRequested, m_priceAlertService, &PriceAlertService::deleteAlert);
    connect(m_alertsPanel, &AlertsPanel::alertActiveToggled, m_priceAlertService,
            &PriceAlertService::setAlertActive);
    connect(m_alertsPanel, &AlertsPanel::dismissTriggerRequested, m_priceAlertService,
            &PriceAlertService::dismissTrigger);

    connect(&m_client, &SupabaseClient::sessionExpired, this, &MainWindow::onSessionExpired);

    refreshData();
    m_priceAlertService->fetchUndismissedTriggers();
}

void MainWindow::setupUi()
{
    // Branded header atop the nav rail, mirroring commodity-hub.eu's sidebar.
    auto *logoTile = new QLabel(QStringLiteral("\U0001F4C8"), this); // chart-increasing
    logoTile->setObjectName(QStringLiteral("logoTile"));
    logoTile->setAlignment(Qt::AlignCenter);
    logoTile->setFixedSize(36, 36);

    auto *appNameLabel = new QLabel(QStringLiteral("Commodity Hub"), this);
    appNameLabel->setObjectName(QStringLiteral("chartTitle"));
    auto *appTaglineLabel = new QLabel(tr("Markets & Analytics"), this);
    appTaglineLabel->setObjectName(QStringLiteral("pageSubtitle"));

    auto *appNameColumn = new QVBoxLayout;
    appNameColumn->setSpacing(0);
    appNameColumn->addWidget(appNameLabel);
    appNameColumn->addWidget(appTaglineLabel);

    auto *sidebarHeaderRow = new QHBoxLayout;
    sidebarHeaderRow->setContentsMargins(14, 14, 14, 10);
    sidebarHeaderRow->setSpacing(10);
    sidebarHeaderRow->addWidget(logoTile);
    sidebarHeaderRow->addLayout(appNameColumn, 1);

    auto *sidebarHeader = new QWidget(this);
    sidebarHeader->setLayout(sidebarHeaderRow);

    m_navList = new QListWidget(this);
    m_navList->setObjectName(QStringLiteral("navRail"));
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setFixedWidth(220);
    m_navList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto *sidebarColumn = new QVBoxLayout;
    sidebarColumn->setContentsMargins(0, 0, 0, 0);
    sidebarColumn->setSpacing(0);
    sidebarColumn->addWidget(sidebarHeader);
    sidebarColumn->addWidget(m_navList, 1);

    m_dashboardPanel = new DashboardPanel(this);
    m_watchlistPanel = new WatchlistPanel(this);
    m_portfolioPanel = new PortfolioPanel(this);
    m_alertsPanel = new AlertsPanel(this);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(m_dashboardPanel);
    m_pages->addWidget(m_watchlistPanel);
    m_pages->addWidget(m_portfolioPanel);
    m_pages->addWidget(m_alertsPanel);

    connect(m_navList, &QListWidget::currentRowChanged, this, &MainWindow::onNavRowChanged);

    m_errorBanner = new QLabel(this);
    m_errorBanner->setObjectName(QStringLiteral("errorBanner"));
    m_errorBanner->setAttribute(Qt::WA_StyledBackground, true);
    m_errorBanner->setWordWrap(true);
    m_errorBanner->setCursor(Qt::PointingHandCursor);
    m_errorBanner->setToolTip(tr("Click to dismiss"));
    m_errorBanner->hide();
    m_errorBanner->installEventFilter(this);

    auto *contentColumn = new QVBoxLayout;
    contentColumn->setContentsMargins(0, 0, 0, 0);
    contentColumn->setSpacing(0);
    contentColumn->addWidget(m_errorBanner);
    contentColumn->addWidget(m_pages, 1);

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(sidebarColumn);
    layout->addLayout(contentColumn, 1);

    setCentralWidget(central);

    m_loadingIndicator = new QProgressBar(this);
    m_loadingIndicator->setObjectName(QStringLiteral("loadingIndicator"));
    m_loadingIndicator->setRange(0, 0); // indeterminate
    m_loadingIndicator->setFixedWidth(120);
    m_loadingIndicator->setFixedHeight(6);
    m_loadingIndicator->setTextVisible(false);
    m_loadingIndicator->hide();
    statusBar()->addPermanentWidget(m_loadingIndicator);

    rebuildNavList();
}

void MainWindow::setupMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *refreshAction = fileMenu->addAction(tr("&Refresh"));
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshData);

    fileMenu->addSeparator();

    QAction *logoutAction = fileMenu->addAction(tr("&Log Out"));
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    struct NavShortcut
    {
        const char *label;
        int pageIndex;
        int key;
    };
    const NavShortcut navShortcuts[] = {
        {"&Overview", PageOverview, Qt::Key_1},
        {"&Watchlist", PageWatchlist, Qt::Key_2},
        {"&Portfolio", PagePortfolio, Qt::Key_3},
        {"&Alerts", PageAlerts, Qt::Key_4},
    };
    for (const NavShortcut &nav : navShortcuts) {
        QAction *action = viewMenu->addAction(tr(nav.label));
        action->setShortcut(QKeySequence(Qt::CTRL | nav.key));
        connect(action, &QAction::triggered, this,
                [this, pageIndex = nav.pageIndex]() { selectPage(pageIndex); });
    }
}

void MainWindow::restoreWindowState()
{
    QSettings settings = makeSettings();
    settings.beginGroup(QStringLiteral("window"));
    const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    else
        resize(1200, 760);

    const QString navKey = settings.value(QStringLiteral("navKey"), QStringLiteral("page:0")).toString();
    settings.endGroup();

    selectNavByKey(navKey);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_errorBanner && event->type() == QEvent::MouseButtonPress) {
        m_errorBanner->hide();
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings = makeSettings();
    settings.beginGroup(QStringLiteral("window"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("navKey"), currentNavSelectionKey());
    settings.endGroup();

    QMainWindow::closeEvent(event);
}

void MainWindow::onCommoditySelected(const QString &commodityName)
{
    m_selectedCommodity = commodityName;
    m_watchlistPanel->chartPanel()->setCommodityName(commodityName);
    m_ohlcService->fetchHistory(commodityName, m_selectedTimeframe);

    m_watchlistPanel->newsPanel()->setCommodityName(commodityName);
    m_newsService->fetchNews(commodityName);
}

void MainWindow::selectCommodityAndShowChart(const QString &commodityName)
{
    selectPage(PageWatchlist);
    onCommoditySelected(commodityName);
}

void MainWindow::rebuildNavList()
{
    const QString previousKey = m_navRows.isEmpty() ? QStringLiteral("page:0") : currentNavSelectionKey();

    m_navList->blockSignals(true);
    m_navList->clear();
    m_navRows.clear();

    auto addHeader = [this](const QString &label) {
        auto *item = new QListWidgetItem(label, m_navList);
        item->setFlags(Qt::NoItemFlags);
        m_navList->addItem(item);
        m_navRows.append({QStringLiteral("header"), QString(), -1});
    };
    auto addCategory = [this](const QString &icon, const QString &category, int count) {
        auto *item =
            new QListWidgetItem(QStringLiteral("%1  %2  (%3)").arg(icon, capitalize(category)).arg(count), m_navList);
        m_navList->addItem(item);
        m_navRows.append({QStringLiteral("category"), category, -1});
    };
    auto addPage = [this](const QString &icon, const QString &label, int pageIndex) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2").arg(icon, label), m_navList);
        m_navList->addItem(item);
        m_navRows.append({QStringLiteral("page"), QString(), pageIndex});
    };
    // Visual placeholders mirroring commodity-hub.eu's Pro/Insights nav — no
    // feature lives behind these yet, so clicking just surfaces a "coming
    // soon" status message instead of switching pages.
    auto addPlaceholder = [this](const QString &icon, const QString &label) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2").arg(icon, label), m_navList);
        item->setToolTip(tr("Coming soon"));
        m_navList->addItem(item);
        m_navRows.append({QStringLiteral("placeholder"), label, -1});
    };

    addHeader(tr("MARKETS"));

    QHash<QString, int> counts;
    for (const Commodity &c : m_commodities)
        counts[c.category] += 1;

    QVector<QString> orderedCategories;
    for (const auto &entry : canonicalCategories()) {
        if (counts.contains(entry.first))
            orderedCategories.append(entry.first);
    }
    QStringList leftover(counts.keys());
    std::sort(leftover.begin(), leftover.end());
    for (const QString &category : std::as_const(leftover)) {
        if (!orderedCategories.contains(category))
            orderedCategories.append(category);
    }

    for (const QString &category : std::as_const(orderedCategories)) {
        QString icon = QStringLiteral("•");
        for (const auto &entry : canonicalCategories()) {
            if (entry.first == category) {
                icon = entry.second;
                break;
            }
        }
        addCategory(icon, category, counts.value(category));
    }

    addHeader(tr("TOOLS"));
    addPage(QStringLiteral("\U0001F4CA"), tr("Overview"), PageOverview);
    addPage(QStringLiteral("★"), tr("Watchlist"), PageWatchlist);
    addPage(QStringLiteral("\U0001F4BC"), tr("Portfolio"), PagePortfolio);
    addPage(QStringLiteral("\U0001F514"), tr("Alerts"), PageAlerts);

    addHeader(tr("PRO"));
    addPlaceholder(QStringLiteral("\U0001F4CA"), tr("Analytics Workspace"));
    addPlaceholder(QStringLiteral("\U0001F4F0"), tr("Daily Brief"));
    addPlaceholder(QStringLiteral("\U0001F4C5"), tr("Seasonality"));
    addPlaceholder(QStringLiteral("\U000021C4"), tr("Spread Monitor"));
    addPlaceholder(QStringLiteral("\U0001F3AF"), tr("Regime Scanner"));
    addPlaceholder(QStringLiteral("\U0001F967"), tr("Portfolio Analytics"));
    addPlaceholder(QStringLiteral("\U0001F9EA"), tr("Backtest Sandbox"));
    addPlaceholder(QStringLiteral("\U0001F4C8"), tr("Forward Curves"));
    addPlaceholder(QStringLiteral("\U0001F9EE"), tr("Spread Calculator"));
    addPlaceholder(QStringLiteral("\U0001F465"), tr("COT Reports"));
    addPlaceholder(QStringLiteral("\U000021C5"), tr("Roll Yield Scanner"));
    addPlaceholder(QStringLiteral("\U0001F30A"), tr("Volatility Cone"));
    addPlaceholder(QStringLiteral("\U0001F4DA"), tr("Term Structure Shift"));

    addHeader(tr("INSIGHTS"));
    addPlaceholder(QStringLiteral("\U0001F4A1"), tr("Expert Insights"));
    addPlaceholder(QStringLiteral("\U0001F393"), tr("Learning Hub"));
    addPlaceholder(QStringLiteral("\U0001F4E1"), tr("Market Sentiment"));

    m_navList->blockSignals(false);

    selectNavByKey(previousKey);
}

QString MainWindow::currentNavSelectionKey() const
{
    const int row = m_navList->currentRow();
    if (row < 0 || row >= m_navRows.size())
        return QStringLiteral("page:0");

    const NavRow &nav = m_navRows.at(row);
    if (nav.type == QStringLiteral("category"))
        return QStringLiteral("category:") + nav.category;
    if (nav.type == QStringLiteral("page"))
        return QStringLiteral("page:") + QString::number(nav.pageIndex);
    return QStringLiteral("page:0");
}

void MainWindow::selectNavByKey(const QString &key)
{
    for (int i = 0; i < m_navRows.size(); ++i) {
        const NavRow &nav = m_navRows.at(i);
        QString rowKey;
        if (nav.type == QStringLiteral("category"))
            rowKey = QStringLiteral("category:") + nav.category;
        else if (nav.type == QStringLiteral("page"))
            rowKey = QStringLiteral("page:") + QString::number(nav.pageIndex);
        else
            continue;

        if (rowKey == key) {
            m_navList->setCurrentRow(i);
            return;
        }
    }

    // Previous selection no longer exists (e.g. commodities haven't loaded
    // yet, or a category disappeared from the catalog) — fall back to Overview.
    selectPage(PageOverview);
}

void MainWindow::selectPage(int pageIndex)
{
    for (int i = 0; i < m_navRows.size(); ++i) {
        if (m_navRows.at(i).type == QStringLiteral("page") && m_navRows.at(i).pageIndex == pageIndex) {
            m_navList->setCurrentRow(i);
            return;
        }
    }
}

void MainWindow::onNavRowChanged(int row)
{
    if (row < 0 || row >= m_navRows.size())
        return;

    const NavRow &nav = m_navRows.at(row);
    if (nav.type == QStringLiteral("category")) {
        m_pages->setCurrentIndex(PageWatchlist);
        m_watchlistPanel->setBrowseCategory(nav.category);
    } else if (nav.type == QStringLiteral("page")) {
        m_pages->setCurrentIndex(nav.pageIndex);
        if (nav.pageIndex == PageWatchlist)
            m_watchlistPanel->showMyWatchlistMode();
    } else if (nav.type == QStringLiteral("placeholder")) {
        statusBar()->showMessage(tr("%1 — coming soon").arg(nav.category), 3000);
    }
}

void MainWindow::onTimeframeChanged(const QString &timeframe)
{
    m_selectedTimeframe = timeframe;
    if (!m_selectedCommodity.isEmpty())
        m_ohlcService->fetchHistory(m_selectedCommodity, m_selectedTimeframe);
}

void MainWindow::refreshData()
{
    beginRequest();
    m_commodityService->fetchAll();
    beginRequest();
    m_watchlistService->fetchWatchlists();
    // Positions are fetched once portfolios come back and PortfolioPanel
    // re-emits portfolioSelected for whichever one is selected (see setPortfolios).
    beginRequest();
    m_portfolioService->fetchPortfolios();
    beginRequest();
    m_priceAlertService->fetchAlerts();
}

void MainWindow::onLogout()
{
    m_session.clear();
    qApp->quit();
}

void MainWindow::onSessionExpired()
{
    // The refresh token was rejected (expired/revoked elsewhere); fall back to
    // asking the user to sign in again rather than leaving every panel stuck
    // showing stale data and silent request failures.
    LoginDialog loginDialog(m_client, this);
    if (loginDialog.exec() != QDialog::Accepted) {
        qApp->quit();
        return;
    }

    setWindowTitle(QStringLiteral("%1 — %2").arg(Config::ApplicationName, m_session.email()));
    refreshData();
}

void MainWindow::beginRequest()
{
    ++m_pendingRequests;
    m_loadingIndicator->setVisible(true);
}

void MainWindow::endRequest()
{
    m_pendingRequests = std::max(0, m_pendingRequests - 1);
    if (m_pendingRequests == 0)
        m_loadingIndicator->setVisible(false);
}

void MainWindow::showError(const QString &context, const QString &message)
{
    m_errorBanner->setText(tr("%1: %2  (click to dismiss)").arg(context, message));
    m_errorBanner->show();

    // Auto-dismiss after a while so a transient failure doesn't permanently
    // occupy the banner; a fresh error restarts the timer via this same call.
    QTimer::singleShot(8000, m_errorBanner, [this]() { m_errorBanner->hide(); });
}
