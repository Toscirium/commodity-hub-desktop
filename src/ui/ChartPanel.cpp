#include "ChartPanel.h"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QStandardItemModel>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace {
constexpr int ChartTypeCandlestickIndex = 1;
}

ChartPanel::ChartPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_titleLabel = new QLabel(tr("Select a commodity"), this);
    m_titleLabel->setObjectName("chartTitle");

    m_timeframeCombo = new QComboBox(this);
    m_timeframeCombo->addItem(tr("1D"), QStringLiteral("1d"));
    m_timeframeCombo->addItem(tr("1M"), QStringLiteral("1m"));
    m_timeframeCombo->addItem(tr("3M"), QStringLiteral("3m"));
    m_timeframeCombo->addItem(tr("6M"), QStringLiteral("6m"));
    m_timeframeCombo->addItem(tr("1Y"), QStringLiteral("1y"));
    m_timeframeCombo->addItem(tr("2Y"), QStringLiteral("2y"));
    m_timeframeCombo->setCurrentIndex(2);

    m_chartTypeCombo = new QComboBox(this);
    m_chartTypeCombo->addItem(tr("Line"), QStringLiteral("line"));
    m_chartTypeCombo->addItem(tr("Candlestick"), QStringLiteral("candlestick"));

    // Candlestick/line rendering itself is TradingView's lightweight-charts,
    // vendored under resources/tradingview/ and hosted by chart.html - see
    // that file for the JS side (window.setBars/clearData/setChartType).
    m_webView = new QWebEngineView(this);
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    m_webView->load(QUrl(QStringLiteral("qrc:/tradingview/chart.html")));
    connect(m_webView, &QWebEngineView::loadFinished, this, &ChartPanel::onPageLoadFinished);

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(m_titleLabel, 1);
    headerRow->addWidget(m_chartTypeCombo);
    headerRow->addWidget(m_timeframeCombo);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addLayout(headerRow);
    layout->addWidget(m_webView, 1);

    connect(m_timeframeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onTimeframeChanged);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onChartTypeChanged);
}

void ChartPanel::runJs(const QString &js)
{
    if (m_pageReady)
        m_webView->page()->runJavaScript(js);
    else
        m_pendingJs.append(js);
}

void ChartPanel::onPageLoadFinished(bool ok)
{
    if (!ok)
        return;

    m_pageReady = true;
    const QVector<QString> pending = m_pendingJs;
    m_pendingJs.clear();
    for (const QString &js : pending)
        m_webView->page()->runJavaScript(js);
}

void ChartPanel::setCommodityName(const QString &name)
{
    m_commodityName = name;
    m_titleLabel->setText(name);
    runJs(QStringLiteral("window.clearData();"));
}

void ChartPanel::setBars(const QString &commodityName, const QVector<OhlcBar> &bars, bool ohlcAvailable)
{
    if (commodityName != m_commodityName)
        return;

    m_ohlcAvailable = ohlcAvailable;
    applyChartTypeAvailability();

    QJsonArray candles;
    QJsonArray line;
    for (const OhlcBar &bar : bars) {
        QDateTime dt = QDateTime::fromString(bar.date, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(bar.date, QStringLiteral("yyyy-MM-dd"));
        if (!dt.isValid())
            continue;

        const qint64 timeSecs = dt.toSecsSinceEpoch();

        QJsonObject candle;
        candle["time"] = timeSecs;
        candle["open"] = bar.open;
        candle["high"] = bar.high;
        candle["low"] = bar.low;
        candle["close"] = bar.close;
        candles.append(candle);

        QJsonObject point;
        point["time"] = timeSecs;
        point["value"] = bar.close;
        line.append(point);
    }

    const QString candlesJson = QString::fromUtf8(QJsonDocument(candles).toJson(QJsonDocument::Compact));
    const QString lineJson = QString::fromUtf8(QJsonDocument(line).toJson(QJsonDocument::Compact));
    runJs(QStringLiteral("window.setBars(%1, %2);").arg(candlesJson, lineJson));
}

void ChartPanel::applyChartTypeAvailability()
{
    auto *model = qobject_cast<QStandardItemModel *>(m_chartTypeCombo->model());
    if (model) {
        if (QStandardItem *item = model->item(ChartTypeCandlestickIndex))
            item->setEnabled(m_ohlcAvailable);
    }

    if (!m_ohlcAvailable && m_chartTypeCombo->currentIndex() == ChartTypeCandlestickIndex) {
        m_chartTypeCombo->blockSignals(true);
        m_chartTypeCombo->setCurrentIndex(0);
        m_chartTypeCombo->blockSignals(false);
        runJs(QStringLiteral("window.setChartType('line');"));
    }
}

void ChartPanel::onTimeframeChanged(int index)
{
    emit timeframeChanged(m_timeframeCombo->itemData(index).toString());
}

void ChartPanel::onChartTypeChanged(int index)
{
    const bool wantsCandlestick = index == ChartTypeCandlestickIndex && m_ohlcAvailable;
    runJs(QStringLiteral("window.setChartType('%1');").arg(wantsCandlestick ? QStringLiteral("candlestick") : QStringLiteral("line")));
}
