#include "ScreenerPanel.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>
#include <algorithm>

#include "CommodityTableModel.h"

namespace {
QString capitalize(const QString &text)
{
    if (text.isEmpty())
        return text;
    return text.at(0).toUpper() + text.mid(1);
}
}

CommodityFilterProxyModel::CommodityFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void CommodityFilterProxyModel::setSearchText(const QString &text)
{
    m_searchText = text;
    invalidateFilter();
}

void CommodityFilterProxyModel::setCategory(const QString &category)
{
    m_category = category;
    invalidateFilter();
}

bool CommodityFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_category.isEmpty() && m_category != QStringLiteral("all")) {
        const QModelIndex categoryIndex = sourceModel()->index(sourceRow, CommodityTableModel::ColumnCategory, sourceParent);
        if (sourceModel()->data(categoryIndex).toString().compare(m_category, Qt::CaseInsensitive) != 0)
            return false;
    }

    if (!m_searchText.isEmpty()) {
        const QModelIndex nameIndex = sourceModel()->index(sourceRow, CommodityTableModel::ColumnName, sourceParent);
        const QModelIndex symbolIndex = sourceModel()->index(sourceRow, CommodityTableModel::ColumnSymbol, sourceParent);
        const QString name = sourceModel()->data(nameIndex).toString();
        const QString symbol = sourceModel()->data(symbolIndex).toString();
        if (!name.contains(m_searchText, Qt::CaseInsensitive) && !symbol.contains(m_searchText, Qt::CaseInsensitive))
            return false;
    }

    return true;
}

bool CommodityFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (CommodityTableModel::isNumericColumn(left.column()))
        return sourceModel()->data(left, Qt::EditRole).toDouble() < sourceModel()->data(right, Qt::EditRole).toDouble();

    return sourceModel()->data(left, Qt::DisplayRole).toString().localeAwareCompare(
               sourceModel()->data(right, Qt::DisplayRole).toString())
        < 0;
}

ScreenerPanel::ScreenerPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *iconTile = new QLabel(QStringLiteral("\U0001F50D"), this); // magnifying glass
    iconTile->setObjectName(QStringLiteral("logoTile"));
    iconTile->setAlignment(Qt::AlignCenter);
    iconTile->setFixedSize(36, 36);

    auto *eyebrow = new QLabel(tr("TOOLS"), this);
    eyebrow->setObjectName(QStringLiteral("panelHeading"));
    auto *heading = new QLabel(tr("Market Screener"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("Search and sort the full commodity catalog"), this);
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

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search symbol or name…"));
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem(tr("All Categories"), QStringLiteral("all"));
    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet(QStringLiteral("color:#898d94;font-size:11px;"));

    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(10);
    filterRow->addWidget(m_searchEdit, 1);
    filterRow->addWidget(m_categoryCombo);
    filterRow->addWidget(m_countLabel);

    m_model = new CommodityTableModel(this);
    m_proxyModel = new CommodityFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(CommodityTableModel::ColumnChangePercent, Qt::DescendingOrder);

    m_emptyLabel = new QLabel(tr("No commodities match your filters."), this);
    m_emptyLabel->setObjectName(QStringLiteral("emptyState"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    layout->addLayout(headerRow);
    layout->addLayout(filterRow);
    layout->addWidget(m_tableView, 1);
    layout->addWidget(m_emptyLabel, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_proxyModel->setSearchText(text);
        m_emptyLabel->setVisible(m_proxyModel->rowCount() == 0 && !m_commodities.isEmpty());
        m_countLabel->setText(tr("%1 of %2").arg(m_proxyModel->rowCount()).arg(m_commodities.size()));
    });
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_proxyModel->setCategory(m_categoryCombo->itemData(index).toString());
        m_emptyLabel->setVisible(m_proxyModel->rowCount() == 0 && !m_commodities.isEmpty());
        m_countLabel->setText(tr("%1 of %2").arg(m_proxyModel->rowCount()).arg(m_commodities.size()));
    });
    connect(m_tableView, &QTableView::activated, this, &ScreenerPanel::onRowActivated);
}

void ScreenerPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;
    m_model->setCommodities(commodities);
    rebuildCategoryFilter();

    const bool empty = commodities.isEmpty();
    m_emptyLabel->setVisible(m_proxyModel->rowCount() == 0 && !empty);
    m_countLabel->setText(tr("%1 of %2").arg(m_proxyModel->rowCount()).arg(commodities.size()));
}

void ScreenerPanel::rebuildCategoryFilter()
{
    QStringList categories;
    for (const Commodity &c : m_commodities) {
        if (!categories.contains(c.category))
            categories.append(c.category);
    }
    std::sort(categories.begin(), categories.end());

    const QString previousCategory = m_categoryCombo->currentData().toString();

    m_categoryCombo->blockSignals(true);
    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("All Categories"), QStringLiteral("all"));
    for (const QString &category : std::as_const(categories))
        m_categoryCombo->addItem(capitalize(category), category);

    const int previousIndex = m_categoryCombo->findData(previousCategory);
    m_categoryCombo->setCurrentIndex(previousIndex >= 0 ? previousIndex : 0);
    m_categoryCombo->blockSignals(false);

    m_proxyModel->setCategory(m_categoryCombo->currentData().toString());
}

void ScreenerPanel::onRowActivated(const QModelIndex &proxyIndex)
{
    const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid())
        return;
    emit commoditySelected(m_model->commodityAt(sourceIndex.row()).name);
}
