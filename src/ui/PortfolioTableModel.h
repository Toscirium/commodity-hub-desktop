#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "models/PortfolioPosition.h"

// Positions joined with the current market price looked up from the
// commodities cache, so the view can show live value/return without a
// separate per-position network call.
class PortfolioTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Row
    {
        PortfolioPosition position;
        double currentPrice = 0.0;
        bool priceKnown = false;
    };

    explicit PortfolioTableModel(QObject *parent = nullptr);

    void setRows(const QVector<Row> &rows);
    const PortfolioPosition &positionAt(int row) const { return m_rows.at(row).position; }
    const Row &rowAt(int row) const { return m_rows.at(row); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QVector<Row> m_rows;
};
