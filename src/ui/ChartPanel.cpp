#include "ChartPanel.h"

#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
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

    m_popoutButton = new QPushButton(tr("Pop Out"), this);
    m_popoutButton->setObjectName(QStringLiteral("secondaryButton"));
    m_popoutButton->setToolTip(tr("Open this chart in a separate window"));

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(m_titleLabel, 1);
    headerRow->addWidget(m_chartTypeCombo);
    headerRow->addWidget(m_timeframeCombo);
    headerRow->addWidget(m_popoutButton);

    m_popoutPlaceholder = new QLabel(tr("Chart opened in a separate window."), this);
    m_popoutPlaceholder->setObjectName(QStringLiteral("emptyState"));
    m_popoutPlaceholder->setAlignment(Qt::AlignCenter);
    m_popoutPlaceholder->hide();

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(12, 12, 12, 12);
    m_layout->setSpacing(8);
    m_layout->addLayout(headerRow);
    m_layout->addWidget(m_webView, 1);
    m_layout->addWidget(m_popoutPlaceholder, 1);

    connect(m_timeframeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onTimeframeChanged);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onChartTypeChanged);
    connect(m_popoutButton, &QPushButton::clicked, this, &ChartPanel::onPopoutClicked);
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
    if (m_popoutWindow)
        m_popoutWindow->setWindowTitle(name);
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

void ChartPanel::onPopoutClicked()
{
    if (m_detached) {
        dockChart();
        return;
    }

    if (!m_popoutWindow) {
        // Qt::Window with a non-null parent: still a real top-level OS
        // window (draggable to another monitor, independent taskbar entry),
        // but owned by this ChartPanel so it's destroyed along with it
        // rather than needing to outlive it.
        m_popoutWindow = new QWidget(this, Qt::Window);
        m_popoutWindow->resize(720, 480);
        auto *popoutLayout = new QVBoxLayout(m_popoutWindow);
        popoutLayout->setContentsMargins(0, 0, 0, 0);
        m_popoutWindow->installEventFilter(this);
    }
    m_popoutWindow->setWindowTitle(m_commodityName.isEmpty() ? tr("Chart") : m_commodityName);

    m_layout->removeWidget(m_webView);
    m_popoutWindow->layout()->addWidget(m_webView);
    m_webView->show();
    m_popoutWindow->show();
    m_popoutWindow->raise();
    m_popoutWindow->activateWindow();

    m_popoutPlaceholder->show();
    m_popoutButton->setText(tr("Dock"));
    m_popoutButton->setToolTip(tr("Bring the chart back into this window"));
    m_detached = true;
}

void ChartPanel::dockChart()
{
    if (!m_detached)
        return;

    m_popoutWindow->layout()->removeWidget(m_webView);
    m_popoutWindow->hide();

    m_layout->insertWidget(1, m_webView, 1);
    m_webView->show();

    m_popoutPlaceholder->hide();
    m_popoutButton->setText(tr("Pop Out"));
    m_popoutButton->setToolTip(tr("Open this chart in a separate window"));
    m_detached = false;
}

bool ChartPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_popoutWindow && event->type() == QEvent::Close)
        dockChart();
    return QWidget::eventFilter(watched, event);
}
