#pragma once

#include <QWidget>
#include <QVector>

#include "models/Commodity.h"
#include "models/Portfolio.h"
#include "models/PortfolioPosition.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTableView;
class PortfolioTableModel;

class PortfolioPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PortfolioPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);
    void setPortfolios(const QVector<Portfolio> &portfolios);
    void setPositions(const QVector<PortfolioPosition> &positions);

signals:
    void portfolioSelected(const QString &portfolioId);
    void createPortfolioRequested(const QString &name);
    void addPositionRequested(const QString &commodityName, double quantity, double entryPrice,
                               const QString &entryDate, const QString &notes, const QString &portfolioId);
    void editPositionRequested(const QString &positionId, const QString &commodityName, double quantity,
                                double entryPrice, const QString &entryDate, const QString &notes);
    void removePositionRequested(const QString &positionId);

private:
    struct Totals
    {
        double value = 0.0;
        double cost = 0.0;
        bool allPricesKnown = true;
    };

    void refreshModel();
    void refreshSummary();
    Totals computeTotals() const;
    void onPortfolioChanged(int index);
    void onNewPortfolioClicked();
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void onExportClicked();

    QVector<Commodity> m_commodities;
    QVector<Portfolio> m_portfolios;
    QVector<PortfolioPosition> m_positions;

    QComboBox *m_portfolioCombo;
    QPushButton *m_newPortfolioButton;
    QLabel *m_totalValueLabel;
    QLabel *m_totalCostLabel;
    QLabel *m_totalReturnLabel;
    QTableView *m_tableView;
    QLabel *m_emptyLabel;
    PortfolioTableModel *m_model;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_removeButton;
    QPushButton *m_exportButton;
};
