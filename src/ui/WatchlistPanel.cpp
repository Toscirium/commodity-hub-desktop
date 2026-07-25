#include "WatchlistPanel.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "CategoryHeaderWidget.h"
#include "ChartPanel.h"
#include "CommodityCardWidget.h"

namespace {
QString capitalize(const QString &text)
{
    if (text.isEmpty())
        return text;
    return text.at(0).toUpper() + text.mid(1);
}
}

WatchlistPanel::WatchlistPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_heading = new QLabel(tr("Watchlist"), this);
    m_heading->setObjectName(QStringLiteral("pageTitle"));
    m_subtitle = new QLabel(tr("Track the commodities you care about"), this);
    m_subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    m_subtitle->setWordWrap(true);

    m_watchlistCombo = new QComboBox(this);
    m_newWatchlistButton = new QPushButton(tr("New List"), this);
    m_newWatchlistButton->setObjectName(QStringLiteral("secondaryButton"));
    m_addCombo = new QComboBox(this);
    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSpacing(6);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_addButton = new QPushButton(tr("Add"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    m_removeButton->setObjectName(QStringLiteral("destructiveButton"));

    // Embedded inline in m_list, directly under whichever card is selected
    // (see showChartBelowItem), rather than living in a side-by-side panel.
    m_chartPanel = new ChartPanel(this);
    m_chartPanel->setMinimumHeight(380);

    auto *watchlistRow = new QHBoxLayout;
    watchlistRow->addWidget(m_watchlistCombo, 1);
    watchlistRow->addWidget(m_newWatchlistButton);

    auto *addRow = new QHBoxLayout;
    addRow->addWidget(m_addCombo, 1);
    addRow->addWidget(m_addButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(m_heading);
    layout->addWidget(m_subtitle);
    layout->addLayout(watchlistRow);
    layout->addWidget(m_list, 1);
    layout->addLayout(addRow);
    layout->addWidget(m_removeButton);

    connect(m_watchlistCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WatchlistPanel::onWatchlistChanged);
    connect(m_list, &QListWidget::currentRowChanged, this, &WatchlistPanel::onRowActivated);
    connect(m_addButton, &QPushButton::clicked, this, &WatchlistPanel::onAddClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &WatchlistPanel::onRemoveClicked);
    connect(m_newWatchlistButton, &QPushButton::clicked, this, &WatchlistPanel::onNewWatchlistClicked);

    m_addCombo->setEnabled(false);
    m_addButton->setEnabled(false);
    m_removeButton->setEnabled(false);
}

void WatchlistPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;

    m_addCombo->clear();
    for (const Commodity &c : m_commodities)
        m_addCombo->addItem(c.name, c.symbol);

    refreshModel();
}

void WatchlistPanel::setWatchlists(const QVector<Watchlist> &watchlists)
{
    const QString previousId = m_watchlistCombo->currentData().toString();
    m_watchlists = watchlists;

    m_watchlistCombo->blockSignals(true);
    m_watchlistCombo->clear();
    int indexToSelect = 0;
    for (int i = 0; i < m_watchlists.size(); ++i) {
        const Watchlist &wl = m_watchlists.at(i);
        m_watchlistCombo->addItem(wl.name, wl.id);
        if (wl.id == previousId)
            indexToSelect = i;
    }
    if (m_watchlistCombo->count() > 0)
        m_watchlistCombo->setCurrentIndex(indexToSelect);
    m_watchlistCombo->blockSignals(false);

    const bool hasWatchlist = !m_watchlists.isEmpty();
    m_addCombo->setEnabled(hasWatchlist);
    m_addButton->setEnabled(hasWatchlist);
    m_watchlistCombo->setPlaceholderText(hasWatchlist ? QString() : tr("No watchlists yet"));

    refreshModel();
}

void WatchlistPanel::setBrowseCategory(const QString &category)
{
    m_mode = Mode::BrowseCategory;
    m_browseCategory = category;
    refreshModel();
}

void WatchlistPanel::showMyWatchlistMode()
{
    m_mode = Mode::MyWatchlist;
    m_browseCategory.clear();
    refreshModel();
}

const Watchlist *WatchlistPanel::currentWatchlist() const
{
    const QString id = m_watchlistCombo->currentData().toString();
    for (const Watchlist &wl : m_watchlists) {
        if (wl.id == id)
            return &wl;
    }
    return nullptr;
}

void WatchlistPanel::refreshModel()
{
    m_rows.clear();

    if (m_mode == Mode::BrowseCategory) {
        for (const Commodity &c : m_commodities) {
            if (c.category == m_browseCategory) {
                Row row;
                row.commodity = c;
                m_rows.append(row);
            }
        }

        m_heading->setText(tr("%1 Commodities").arg(capitalize(m_browseCategory)));
        m_subtitle->setText(tr("%1 instrument(s) in this category — pick one to chart it, or add it to a watchlist below")
                                 .arg(m_rows.size()));
    } else {
        if (const Watchlist *wl = currentWatchlist()) {
            for (const WatchlistItem &item : wl->items) {
                Row row;
                row.itemId = item.id;
                row.commodity.name = item.commodityName;
                row.commodity.symbol = item.commoditySymbol;

                for (const Commodity &c : m_commodities) {
                    if (c.name == item.commodityName) {
                        row.commodity = c;
                        break;
                    }
                }
                m_rows.append(row);
            }
        }

        m_heading->setText(tr("Watchlist"));
        m_subtitle->setText(tr("Track the commodities you care about"));
    }

    detachChartPanel();
    m_list->clear();

    int firstCardListRow = -1;

    if (m_mode == Mode::BrowseCategory) {
        // Already filtered to one category, so a repeated section header
        // would just restate the page title above — skip straight to cards.
        for (int rowIndex = 0; rowIndex < m_rows.size(); ++rowIndex) {
            auto *card = new CommodityCardWidget(m_list);
            card->setCommodity(m_rows.at(rowIndex).commodity);

            auto *item = new QListWidgetItem(m_list);
            item->setData(Qt::UserRole, rowIndex);
            item->setSizeHint(card->sizeHint());
            m_list->addItem(item);
            m_list->setItemWidget(item, card);

            if (firstCardListRow < 0)
                firstCardListRow = m_list->row(item);
        }
    } else {
        // Group into category sections (colored accent bar + uppercase label +
        // count), preserving the order categories first appear in rather than
        // alphabetizing, so the grouping stays stable as items are added/removed.
        QVector<QString> categoryOrder;
        QHash<QString, QVector<int>> categoryToRowIndices;
        for (int i = 0; i < m_rows.size(); ++i) {
            QString category = m_rows.at(i).commodity.category;
            if (category.isEmpty())
                category = tr("other");
            if (!categoryToRowIndices.contains(category))
                categoryOrder.append(category);
            categoryToRowIndices[category].append(i);
        }

        for (const QString &category : categoryOrder) {
            const QVector<int> &indices = categoryToRowIndices.value(category);

            auto *header = new CategoryHeaderWidget(category, indices.size(), m_list);
            auto *headerItem = new QListWidgetItem(m_list);
            headerItem->setFlags(Qt::NoItemFlags);
            headerItem->setSizeHint(header->sizeHint());
            m_list->addItem(headerItem);
            m_list->setItemWidget(headerItem, header);

            for (int rowIndex : indices) {
                auto *card = new CommodityCardWidget(m_list);
                card->setCommodity(m_rows.at(rowIndex).commodity);

                auto *item = new QListWidgetItem(m_list);
                item->setData(Qt::UserRole, rowIndex);
                item->setSizeHint(card->sizeHint());
                m_list->addItem(item);
                m_list->setItemWidget(item, card);

                if (firstCardListRow < 0)
                    firstCardListRow = m_list->row(item);
            }
        }
    }

    m_removeButton->setEnabled(false);

    if (firstCardListRow >= 0)
        m_list->setCurrentRow(firstCardListRow);
}

void WatchlistPanel::onWatchlistChanged(int)
{
    if (m_mode == Mode::MyWatchlist)
        refreshModel();
}

void WatchlistPanel::onRowActivated(int row)
{
    // Category header rows carry no UserRole index (see refreshModel) and are
    // Qt::NoItemFlags, so this also naturally no-ops if one somehow becomes current.
    QListWidgetItem *item = m_list->item(row);
    const QVariant data = item ? item->data(Qt::UserRole) : QVariant();
    if (!data.isValid())
        return;

    const int rowIndex = data.toInt();
    if (rowIndex < 0 || rowIndex >= m_rows.size())
        return;

    emit commoditySelected(m_rows.at(rowIndex).commodity.name);
    // Nothing to remove for a browsed (not-yet-watchlisted) commodity.
    m_removeButton->setEnabled(m_mode == Mode::MyWatchlist);

    showChartBelowItem(item);
}

void WatchlistPanel::showChartBelowItem(QListWidgetItem *cardItem)
{
    if (m_chartItem) {
        // Pull the persistent chart panel out before the old row (and the
        // disposable container widget holding it) gets destroyed.
        m_chartPanel->setParent(this);
        delete m_list->takeItem(m_list->row(m_chartItem));
        m_chartItem = nullptr;
    }

    auto *chartItem = new QListWidgetItem;
    chartItem->setFlags(Qt::NoItemFlags);
    chartItem->setSizeHint(QSize(0, 380));
    m_list->insertItem(m_list->row(cardItem) + 1, chartItem);

    // QAbstractItemView::setItemWidget()/removeItemWidget() schedule the
    // *previous* index widget for deleteLater() whenever it's replaced or
    // cleared - it does not just "detach" it. Since m_chartPanel is reused
    // across rows, it must never be handed to setItemWidget() directly (that
    // would get it deleted out from under us on the next event loop tick,
    // leaving MainWindow's async OHLC callback writing into freed memory).
    // Give the view a disposable container instead, and always reparent
    // m_chartPanel back out of it before the container can be torn down.
    auto *container = new QWidget;
    container->setAttribute(Qt::WA_StyledBackground, true);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->setParent(container);
    containerLayout->addWidget(m_chartPanel);
    m_list->setItemWidget(chartItem, container);
    m_chartPanel->show();
    m_chartItem = chartItem;
}

void WatchlistPanel::detachChartPanel()
{
    if (!m_chartItem)
        return;

    // Reparent the persistent chart panel out of its disposable container
    // widget before that container (and the row it lives on) is destroyed.
    m_chartPanel->setParent(this);
    m_chartPanel->hide();
    m_chartItem = nullptr;
}

void WatchlistPanel::onAddClicked()
{
    const Watchlist *wl = currentWatchlist();
    if (!wl || m_addCombo->currentIndex() < 0)
        return;

    emit addItemRequested(wl->id, m_addCombo->currentText(), m_addCombo->currentData().toString());
}

void WatchlistPanel::onRemoveClicked()
{
    QListWidgetItem *item = m_list->item(m_list->currentRow());
    const QVariant data = item ? item->data(Qt::UserRole) : QVariant();
    if (!data.isValid())
        return;

    const int rowIndex = data.toInt();
    if (rowIndex < 0 || rowIndex >= m_rows.size() || m_rows.at(rowIndex).itemId.isEmpty())
        return;

    emit removeItemRequested(m_rows.at(rowIndex).itemId);
}

void WatchlistPanel::onNewWatchlistClicked()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Watchlist"), tr("Name:"),
                                                 QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    emit createWatchlistRequested(name.trimmed());
}
