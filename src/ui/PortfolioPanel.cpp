#include "PortfolioPanel.h"

#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "AddPositionDialog.h"
#include "PortfolioTableModel.h"

namespace {
const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);
}

PortfolioPanel::PortfolioPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *heading = new QLabel(tr("My Portfolio"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("Track your commodity positions and performance"), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_portfolioCombo = new QComboBox(this);
    m_newPortfolioButton = new QPushButton(tr("New Portfolio"), this);
    m_newPortfolioButton->setObjectName(QStringLiteral("secondaryButton"));

    m_totalValueLabel = new QLabel(this);
    m_totalCostLabel = new QLabel(this);
    m_totalReturnLabel = new QLabel(this);
    for (QLabel *label : {m_totalValueLabel, m_totalCostLabel, m_totalReturnLabel}) {
        label->setObjectName(QStringLiteral("card"));
        label->setAttribute(Qt::WA_StyledBackground, true);
        label->setMargin(12);
    }

    m_model = new PortfolioTableModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->setFrameShape(QFrame::NoFrame);

    m_emptyLabel = new QLabel(tr("No positions yet — click “Add Position” to get started."), this);
    m_emptyLabel->setObjectName(QStringLiteral("emptyState"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->hide();

    m_addButton = new QPushButton(tr("Add Position"), this);
    m_editButton = new QPushButton(tr("Edit"), this);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    m_editButton->setEnabled(false);
    m_removeButton = new QPushButton(tr("Remove"), this);
    m_removeButton->setObjectName(QStringLiteral("destructiveButton"));
    m_removeButton->setEnabled(false);

    auto *portfolioRow = new QHBoxLayout;
    portfolioRow->addWidget(m_portfolioCombo, 1);
    portfolioRow->addWidget(m_newPortfolioButton);

    auto *summaryRow = new QHBoxLayout;
    summaryRow->addWidget(m_totalValueLabel, 1);
    summaryRow->addWidget(m_totalCostLabel, 1);
    summaryRow->addWidget(m_totalReturnLabel, 1);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_editButton);
    buttonRow->addWidget(m_removeButton);
    buttonRow->addStretch(1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addLayout(portfolioRow);
    layout->addLayout(summaryRow);
    layout->addWidget(m_tableView, 1);
    layout->addWidget(m_emptyLabel, 1);
    layout->addLayout(buttonRow);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        const bool hasSelection = m_tableView->currentIndex().isValid();
        m_editButton->setEnabled(hasSelection);
        m_removeButton->setEnabled(hasSelection);
    });
    connect(m_portfolioCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &PortfolioPanel::onPortfolioChanged);
    connect(m_newPortfolioButton, &QPushButton::clicked, this, &PortfolioPanel::onNewPortfolioClicked);
    connect(m_addButton, &QPushButton::clicked, this, &PortfolioPanel::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &PortfolioPanel::onEditClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &PortfolioPanel::onRemoveClicked);

    refreshSummary();
}

void PortfolioPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;
    refreshModel();
}

void PortfolioPanel::setPortfolios(const QVector<Portfolio> &portfolios)
{
    const QString previousId = m_portfolioCombo->currentData().toString();
    m_portfolios = portfolios;

    m_portfolioCombo->blockSignals(true);
    m_portfolioCombo->clear();
    int indexToSelect = 0;
    for (int i = 0; i < m_portfolios.size(); ++i) {
        const Portfolio &p = m_portfolios.at(i);
        m_portfolioCombo->addItem(p.name, p.id);
        if (p.id == previousId)
            indexToSelect = i;
    }
    if (m_portfolioCombo->count() > 0)
        m_portfolioCombo->setCurrentIndex(indexToSelect);
    m_portfolioCombo->blockSignals(false);

    // Free tier only ever has one (server-managed) portfolio; hide the picker
    // entirely rather than showing a combo with nothing to switch between.
    m_portfolioCombo->setVisible(m_portfolios.size() > 1);

    // Positions are fetched per-portfolio (unlike watchlists, they aren't
    // embedded), so every rebuild needs to (re-)trigger a fetch for whichever
    // portfolio ended up selected — signals were blocked above specifically to
    // avoid firing this on incidental re-selection, so it's done explicitly here.
    emit portfolioSelected(m_portfolioCombo->currentData().toString());
}

void PortfolioPanel::setPositions(const QVector<PortfolioPosition> &positions)
{
    m_positions = positions;
    refreshModel();
}

void PortfolioPanel::refreshModel()
{
    QVector<PortfolioTableModel::Row> rows;
    rows.reserve(m_positions.size());

    for (const PortfolioPosition &position : m_positions) {
        PortfolioTableModel::Row row;
        row.position = position;
        for (const Commodity &c : m_commodities) {
            if (c.name == position.commodityName) {
                row.currentPrice = c.price;
                row.priceKnown = true;
                break;
            }
        }
        rows.append(row);
    }

    m_model->setRows(rows);
    m_editButton->setEnabled(false);
    m_removeButton->setEnabled(false);

    m_tableView->setVisible(!rows.isEmpty());
    m_emptyLabel->setVisible(rows.isEmpty());

    refreshSummary();
}

void PortfolioPanel::refreshSummary()
{
    double totalValue = 0.0;
    double totalCost = 0.0;
    bool allPricesKnown = true;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        const PortfolioPosition &position = m_model->positionAt(i);
        totalCost += position.quantity * position.entryPrice;

        bool priceKnown = false;
        for (const Commodity &c : m_commodities) {
            if (c.name == position.commodityName) {
                totalValue += position.quantity * c.price;
                priceKnown = true;
                break;
            }
        }
        if (!priceKnown) {
            allPricesKnown = false;
            totalValue += position.quantity * position.entryPrice;
        }
    }

    const double totalReturn = totalValue - totalCost;
    const double returnPct = totalCost > 0.0 ? (totalReturn / totalCost) * 100.0 : 0.0;

    m_totalValueLabel->setText(tr("Total Value\n%1%2").arg(QString::number(totalValue, 'f', 2),
                                                             allPricesKnown ? QString() : tr(" (partial)")));
    m_totalCostLabel->setText(tr("Total Cost\n%1").arg(QString::number(totalCost, 'f', 2)));

    const QString sign = totalReturn >= 0.0 ? "+" : "";
    m_totalReturnLabel->setText(
        tr("Total Return\n%1%2 (%3%4%)").arg(sign, QString::number(totalReturn, 'f', 2), sign,
                                              QString::number(returnPct, 'f', 2)));

    // A QSS rule directly on the widget instance takes precedence over the
    // app-wide `QWidget { color: ... }` rule in theme.qss, so this is enough
    // to tint just this label without fighting the global stylesheet.
    const QColor color = totalReturn >= 0.0 ? kPositiveColor : kNegativeColor;
    m_totalReturnLabel->setStyleSheet(QStringLiteral("QLabel#card { color: %1; }").arg(color.name()));
}

void PortfolioPanel::onPortfolioChanged(int index)
{
    emit portfolioSelected(m_portfolioCombo->itemData(index).toString());
}

void PortfolioPanel::onNewPortfolioClicked()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Portfolio"), tr("Name:"), QLineEdit::Normal,
                                                 tr("My Portfolio"), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    emit createPortfolioRequested(name.trimmed());
}

void PortfolioPanel::onAddClicked()
{
    if (m_commodities.isEmpty())
        return;

    AddPositionDialog dialog(m_commodities, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    emit addPositionRequested(dialog.commodityName(), dialog.quantity(), dialog.entryPrice(), dialog.entryDate(),
                               dialog.notes(), m_portfolioCombo->currentData().toString());
}

void PortfolioPanel::onEditClicked()
{
    const QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid())
        return;

    const PortfolioPosition &existing = m_model->positionAt(index.row());
    AddPositionDialog dialog(m_commodities, this, existing);
    if (dialog.exec() != QDialog::Accepted)
        return;

    emit editPositionRequested(existing.id, dialog.commodityName(), dialog.quantity(), dialog.entryPrice(),
                                dialog.entryDate(), dialog.notes());
}

void PortfolioPanel::onRemoveClicked()
{
    const QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid())
        return;

    emit removePositionRequested(m_model->positionAt(index.row()).id);
}
