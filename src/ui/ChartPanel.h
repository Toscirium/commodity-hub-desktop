#pragma once

#include <QWidget>
#include <QVector>

#include "models/OhlcBar.h"

class QLabel;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QWebEngineView;

// Candlestick/line rendering is TradingView's lightweight-charts (vendored in
// resources/tradingview/), run inside a QWebEngineView rather than Qt Charts —
// see chart.html for the JS side of this. Everything outside the chart canvas
// itself (title, timeframe/chart-type combos) stays plain Qt widgets.
class ChartPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChartPanel(QWidget *parent = nullptr);

    void setCommodityName(const QString &name);
    void setBars(const QString &commodityName, const QVector<OhlcBar> &bars, bool ohlcAvailable);

signals:
    void timeframeChanged(const QString &timeframe);

protected:
    // Watches m_popoutWindow for the user closing it via the OS window
    // controls, so that case re-docks the chart the same as clicking "Dock".
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTimeframeChanged(int index);
    void onChartTypeChanged(int index);
    void onPageLoadFinished(bool ok);
    void onPopoutClicked();

private:
    void applyChartTypeAvailability();
    // Queues js until the page has finished loading (setBars()/setCommodityName()
    // can be called before that, e.g. while the very first OHLC fetch races the
    // WebEngine page load), then runs it immediately once ready.
    void runJs(const QString &js);
    void dockChart();

    QString m_commodityName;
    bool m_ohlcAvailable = true;
    bool m_pageReady = false;
    QVector<QString> m_pendingJs;

    QLabel *m_titleLabel;
    QComboBox *m_timeframeCombo;
    QComboBox *m_chartTypeCombo;
    QWebEngineView *m_webView;

    // The webview is the one heavy (Chromium-backed) resource here, so
    // "popping out" reparents this same QWebEngineView into a separate
    // top-level window rather than spinning up a second one — everywhere
    // else, ChartPanel keeps behaving exactly as before (WatchlistPanel still
    // reparents the whole ChartPanel between watchlist cards; that's fully
    // orthogonal to where its webview currently lives).
    QVBoxLayout *m_layout;
    QPushButton *m_popoutButton;
    QLabel *m_popoutPlaceholder;
    QWidget *m_popoutWindow = nullptr;
    bool m_detached = false;
};
