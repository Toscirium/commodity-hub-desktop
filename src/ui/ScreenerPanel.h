#pragma once

#include <QSortFilterProxyModel>
#include <QVector>
#include <QWidget>

#include "models/Commodity.h"

class CommodityTableModel;
class QLineEdit;
class QComboBox;
class QTableView;
class QLabel;

// Sorts/filters CommodityTableModel by category and by a text search over
// symbol/name; numeric columns sort on their raw value (Qt::EditRole), not
// the formatted display string, so e.g. Price sorts numerically.
class CommodityFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit CommodityFilterProxyModel(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setCategory(const QString &category); // empty or "all" = no filter

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_searchText;
    QString m_category;
};

// "Tools" page listing every commodity in a sortable/filterable table —
// commodity-hub.eu's Market Screener, minus the Pro-gated regime/signal
// columns and CSV export (see commodity-hub/src/pages/MarketScreener.tsx).
// Recomputes entirely from data MainWindow already fetched, so it needs no
// network calls of its own.
class ScreenerPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenerPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);

signals:
    void commoditySelected(const QString &commodityName);

private:
    void rebuildCategoryFilter();
    void onRowActivated(const QModelIndex &proxyIndex);

    QVector<Commodity> m_commodities;
    CommodityTableModel *m_model;
    CommodityFilterProxyModel *m_proxyModel;

    QLineEdit *m_searchEdit;
    QComboBox *m_categoryCombo;
    QTableView *m_tableView;
    QLabel *m_emptyLabel;
    QLabel *m_countLabel;
};
