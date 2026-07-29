#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "models/Commodity.h"

// Flat, sortable table over the full commodity catalog — the data source for
// ScreenerPanel. Unlike CommodityCardWidget/DashboardPanel's rich cards, this
// is deliberately a dense spreadsheet view since screening is a scan-many,
// sort-and-filter workflow rather than a browse-one workflow.
class CommodityTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColumnSymbol = 0,
        ColumnName,
        ColumnCategory,
        ColumnPrice,
        ColumnChange,
        ColumnChangePercent,
        ColumnVolume,
        ColumnCount,
    };

    explicit CommodityTableModel(QObject *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);
    const Commodity &commodityAt(int row) const { return m_commodities.at(row); }

    static bool isNumericColumn(int column);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QVector<Commodity> m_commodities;
};
