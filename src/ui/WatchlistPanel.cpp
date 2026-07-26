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
#include "NewsPanel.h"

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

    // Both embedded inline in m_list, stacked directly under whichever card
    // is expanded (see showDetailsBelowItem), rather than living in a
    // side-by-side panel.
    m_chartPanel = new ChartPanel(this);
    m_chartPanel->setMinimumHeight(380);
    m_newsPanel = new NewsPanel(this);
    m_newsPanel->setMinimumHeight(340);

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
    connect(m_list, &QListWidget::itemClicked, this, &WatchlistPanel::onCardClicked);
    connect(m_list, &QListWidget::itemActivated, this, &WatchlistPanel::onCardClicked);
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

    detachDetailsPanels();
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

    m_expandedRowIndex = -1;
    m_removeButton->setEnabled(false);
    if (firstCardListRow >= 0) {
        m_list->setCurrentRow(firstCardListRow);
        toggleRow(m_list->item(firstCardListRow));
    }
}

void WatchlistPanel::onWatchlistChanged(int)
{
    if (m_mode == Mode::MyWatchlist)
        refreshModel();
}

void WatchlistPanel::onCardClicked(QListWidgetItem *item)
{
    toggleRow(item);
}

void WatchlistPanel::toggleRow(QListWidgetItem *cardItem)
{
    // Category header rows and the chart/news details row itself carry no
    // UserRole index (see refreshModel/showDetailsBelowItem) and are
    // Qt::NoItemFlags, so this naturally no-ops if one somehow gets clicked.
    const QVariant data = cardItem ? cardItem->data(Qt::UserRole) : QVariant();
    if (!data.isValid())
        return;

    const int rowIndex = data.toInt();
    if (rowIndex < 0 || rowIndex >= m_rows.size())
        return;

    m_list->setCurrentItem(cardItem);

    if (rowIndex == m_expandedRowIndex) {
        // Same card clicked again — collapse.
        detachDetailsPanels();
        setCardExpanded(m_expandedRowIndex, false);
        m_expandedRowIndex = -1;
        m_removeButton->setEnabled(false);
        return;
    }

    setCardExpanded(m_expandedRowIndex, false);
    m_expandedRowIndex = rowIndex;
    setCardExpanded(rowIndex, true);

    // Nothing to remove for a browsed (not-yet-watchlisted) commodity.
    m_removeButton->setEnabled(m_mode == Mode::MyWatchlist);

    emit commoditySelected(m_rows.at(rowIndex).commodity.name);
    showDetailsBelowItem(cardItem);
}

CommodityCardWidget *WatchlistPanel::cardWidgetForRow(int rowIndex) const
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        const QVariant data = item->data(Qt::UserRole);
        if (data.isValid() && data.toInt() == rowIndex)
            return qobject_cast<CommodityCardWidget *>(m_list->itemWidget(item));
    }
    return nullptr;
}

void WatchlistPanel::setCardExpanded(int rowIndex, bool expanded)
{
    if (rowIndex < 0)
        return;
    if (CommodityCardWidget *card = cardWidgetForRow(rowIndex))
        card->setExpanded(expanded);
}

void WatchlistPanel::showDetailsBelowItem(QListWidgetItem *cardItem)
{
    // Pull the persistent chart/news panels out before the old row (and the
    // disposable container widget holding them) gets destroyed.
    detachDetailsPanels();

    auto *detailsItem = new QListWidgetItem;
    detailsItem->setFlags(Qt::NoItemFlags);
    detailsItem->setSizeHint(QSize(0, 740));
    m_list->insertItem(m_list->row(cardItem) + 1, detailsItem);

    // QAbstractItemView::setItemWidget()/removeItemWidget() schedule the
    // *previous* index widget for deleteLater() whenever it's replaced or
    // cleared - it does not just "detach" it. Since m_chartPanel/m_newsPanel
    // are reused across rows, they must never be handed to setItemWidget()
    // directly (that would get them deleted out from under us on the next
    // event loop tick, leaving async OHLC/news callbacks writing into freed
    // memory). Give the view a disposable container instead, and always
    // reparent the real panels back out of it before the container is torn
    // down (see detachDetailsPanels()).
    auto *container = new QWidget;
    container->setAttribute(Qt::WA_StyledBackground, true);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(12);
    m_chartPanel->setParent(container);
    m_newsPanel->setParent(container);
    containerLayout->addWidget(m_chartPanel);
    containerLayout->addWidget(m_newsPanel);
    m_list->setItemWidget(detailsItem, container);
    m_chartPanel->show();
    m_newsPanel->show();
    m_detailsItem = detailsItem;
}

void WatchlistPanel::detachDetailsPanels()
{
    if (!m_detailsItem)
        return;

    // Reparent the persistent chart/news panels out of their disposable
    // container widget before that container (and the row it lives on) is
    // destroyed.
    m_chartPanel->setParent(this);
    m_chartPanel->hide();
    m_newsPanel->setParent(this);
    m_newsPanel->hide();
    delete m_list->takeItem(m_list->row(m_detailsItem));
    m_detailsItem = nullptr;
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
    if (m_expandedRowIndex < 0 || m_expandedRowIndex >= m_rows.size() || m_rows.at(m_expandedRowIndex).itemId.isEmpty())
        return;

    emit removeItemRequested(m_rows.at(m_expandedRowIndex).itemId);
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
