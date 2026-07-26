#pragma once

#include <QWidget>
#include <QVector>

#include "models/Commodity.h"
#include "models/Watchlist.h"

class ChartPanel;
class CommodityCardWidget;
class NewsPanel;
class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class WatchlistPanel : public QWidget
{
    Q_OBJECT
public:
    explicit WatchlistPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);
    void setWatchlists(const QVector<Watchlist> &watchlists);

    // Owned by this panel so its chart/news can be embedded inline in the
    // card list (under whichever commodity is currently expanded) instead of
    // living beside it; MainWindow still drives them (feeding OHLC/news
    // data, forwarding timeframe changes) via these accessors.
    ChartPanel *chartPanel() const { return m_chartPanel; }
    NewsPanel *newsPanel() const { return m_newsPanel; }

    // Switches the list from "my watchlist" to browsing every commodity in
    // one category (mirrors clicking a Markets category in the web app's
    // sidebar). Add/remove-from-watchlist still works the same way underneath.
    void setBrowseCategory(const QString &category);
    void showMyWatchlistMode();

signals:
    void commoditySelected(const QString &commodityName);
    void addItemRequested(const QString &watchlistId, const QString &commodityName, const QString &commoditySymbol);
    void removeItemRequested(const QString &itemId);
    void createWatchlistRequested(const QString &name);

private slots:
    void onWatchlistChanged(int index);
    void onCardClicked(QListWidgetItem *item);
    void onAddClicked();
    void onRemoveClicked();
    void onNewWatchlistClicked();

private:
    enum class Mode
    {
        MyWatchlist,
        BrowseCategory,
    };

    struct Row
    {
        QString itemId; // empty in BrowseCategory mode (nothing to remove)
        Commodity commodity;
    };

    void refreshModel();
    const Watchlist *currentWatchlist() const;
    // Expands cardItem's chart/news if some other row (or nothing) was
    // expanded; collapses them if cardItem was already the expanded row.
    // Mirrors CommodityCard.tsx's click-header-to-toggle behavior.
    void toggleRow(QListWidgetItem *cardItem);
    void showDetailsBelowItem(QListWidgetItem *cardItem);
    void detachDetailsPanels();
    CommodityCardWidget *cardWidgetForRow(int rowIndex) const;
    void setCardExpanded(int rowIndex, bool expanded);

    QLabel *m_heading;
    QLabel *m_subtitle;
    QComboBox *m_watchlistCombo;
    QPushButton *m_newWatchlistButton;
    QComboBox *m_addCombo;
    QListWidget *m_list;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    ChartPanel *m_chartPanel;
    NewsPanel *m_newsPanel;
    QListWidgetItem *m_detailsItem = nullptr;
    int m_expandedRowIndex = -1;
    QVector<Row> m_rows;

    QVector<Commodity> m_commodities;
    QVector<Watchlist> m_watchlists;

    Mode m_mode = Mode::MyWatchlist;
    QString m_browseCategory;
};
