#include "PortfolioPanel.h"

#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageLayout>
#include <QPalette>
#include <QPrinter>
#include <QPushButton>
#include <QTableView>
#include <QTextDocument>
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
        label->setMargin(10);
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
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

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
    m_exportButton = new QPushButton(tr("Export PDF"), this);
    m_exportButton->setObjectName(QStringLiteral("secondaryButton"));
    m_exportButton->setEnabled(false);

    auto *portfolioRow = new QHBoxLayout;
    portfolioRow->addWidget(m_portfolioCombo, 1);
    portfolioRow->addWidget(m_newPortfolioButton);

    auto *summaryRow = new QHBoxLayout;
    summaryRow->setSpacing(10);
    summaryRow->addWidget(m_totalValueLabel, 1);
    summaryRow->addWidget(m_totalCostLabel, 1);
    summaryRow->addWidget(m_totalReturnLabel, 1);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_editButton);
    buttonRow->addWidget(m_removeButton);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_exportButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);
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
    connect(m_exportButton, &QPushButton::clicked, this, &PortfolioPanel::onExportClicked);

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
    m_exportButton->setEnabled(!rows.isEmpty());

    m_tableView->setVisible(!rows.isEmpty());
    m_emptyLabel->setVisible(rows.isEmpty());

    refreshSummary();
}

PortfolioPanel::Totals PortfolioPanel::computeTotals() const
{
    Totals totals;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        const PortfolioPosition &position = m_model->positionAt(i);
        totals.cost += position.quantity * position.entryPrice;

        bool priceKnown = false;
        for (const Commodity &c : m_commodities) {
            if (c.name == position.commodityName) {
                totals.value += position.quantity * c.price;
                priceKnown = true;
                break;
            }
        }
        if (!priceKnown) {
            totals.allPricesKnown = false;
            totals.value += position.quantity * position.entryPrice;
        }
    }

    return totals;
}

void PortfolioPanel::refreshSummary()
{
    const Totals totals = computeTotals();
    const double totalReturn = totals.value - totals.cost;
    const double returnPct = totals.cost > 0.0 ? (totalReturn / totals.cost) * 100.0 : 0.0;

    m_totalValueLabel->setText(tr("Total Value\n%1%2").arg(QString::number(totals.value, 'f', 2),
                                                             totals.allPricesKnown ? QString() : tr(" (partial)")));
    m_totalCostLabel->setText(tr("Total Cost\n%1").arg(QString::number(totals.cost, 'f', 2)));

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

void PortfolioPanel::onExportClicked()
{
    if (m_model->rowCount() == 0)
        return;

    const QString portfolioName = m_portfolioCombo->currentText().isEmpty()
        ? tr("Portfolio")
        : m_portfolioCombo->currentText();
    const QString suggestedName =
        QStringLiteral("%1-%2.pdf").arg(portfolioName, QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    const QString filePath =
        QFileDialog::getSaveFileName(this, tr("Export Portfolio Statement"), suggestedName, tr("PDF Files (*.pdf)"));
    if (filePath.isEmpty())
        return;

    QString rowsHtml;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const PortfolioTableModel::Row &row = m_model->rowAt(i);
        const double entryValue = row.position.quantity * row.position.entryPrice;
        const double currentValue = row.priceKnown ? row.position.quantity * row.currentPrice : entryValue;
        const double rowReturn = row.priceKnown ? currentValue - entryValue : 0.0;
        const double rowReturnPct =
            row.priceKnown && row.position.entryPrice != 0.0
            ? (row.currentPrice - row.position.entryPrice) / row.position.entryPrice * 100.0
            : 0.0;
        const QString returnColor = rowReturn >= 0.0 ? QStringLiteral("#1a7f4e") : QStringLiteral("#b3261e");

        rowsHtml += QStringLiteral("<tr>"
                                    "<td>%1</td>"
                                    "<td align='right'>%2</td>"
                                    "<td align='right'>%3</td>"
                                    "<td align='right'>%4</td>"
                                    "<td align='right'>%5</td>"
                                    "<td align='right' style='color:%6'>%7</td>"
                                    "<td align='right' style='color:%6'>%8</td>"
                                    "</tr>")
                         .arg(row.position.commodityName.toHtmlEscaped())
                         .arg(QString::number(row.position.quantity, 'f',
                                               row.position.quantity == int(row.position.quantity) ? 0 : 4))
                         .arg(QString::number(row.position.entryPrice, 'f', 2))
                         .arg(row.priceKnown ? QString::number(row.currentPrice, 'f', 2) : QStringLiteral("—"))
                         .arg(row.priceKnown ? QString::number(currentValue, 'f', 2) : QStringLiteral("—"))
                         .arg(returnColor)
                         .arg(row.priceKnown
                                  ? QStringLiteral("%1%2").arg(rowReturn >= 0.0 ? "+" : "").arg(rowReturn, 0, 'f', 2)
                                  : QStringLiteral("—"))
                         .arg(row.priceKnown ? QStringLiteral("%1%2%")
                                                    .arg(rowReturnPct >= 0.0 ? "+" : "")
                                                    .arg(rowReturnPct, 0, 'f', 2)
                                              : QStringLiteral("—"));
    }

    const Totals totals = computeTotals();
    const double totalReturn = totals.value - totals.cost;
    const double totalReturnPct = totals.cost > 0.0 ? (totalReturn / totals.cost) * 100.0 : 0.0;
    const QString totalReturnColor = totalReturn >= 0.0 ? QStringLiteral("#1a7f4e") : QStringLiteral("#b3261e");
    const QString totalReturnSign = totalReturn >= 0.0 ? QStringLiteral("+") : QString();
    // Built separately (rather than as more %N placeholders below) since Qt's
    // QString::arg() place markers only reliably disambiguate %1..%9 — a %10
    // right after other digit placeholders risks being parsed as %1 + "0".
    const QString totalReturnText = QStringLiteral("%1%2 (%1%3%)")
                                         .arg(totalReturnSign, QString::number(totalReturn, 'f', 2),
                                              QString::number(totalReturnPct, 'f', 2));

    const QString html =
        QStringLiteral(
            "<html><body style='font-family:sans-serif;'>"
            "<h2 style='margin-bottom:2px;'>CommodityHub Portfolio Statement</h2>"
            "<p style='color:#555;margin-top:0;'>%1 &middot; Generated %2%3</p>"
            "<table border='1' cellspacing='0' cellpadding='6' width='100%' style='border-collapse:collapse;'>"
            "<tr style='background:#eee;'>"
            "<th align='left'>Commodity</th><th align='right'>Qty</th><th align='right'>Entry</th>"
            "<th align='right'>Price</th><th align='right'>Value</th><th align='right'>Return</th>"
            "<th align='right'>Return %</th>"
            "</tr>"
            "%4"
            "</table>"
            "<p style='margin-top:16px;'>"
            "Total Cost: <b>%5</b> &nbsp;&nbsp; Total Value: <b>%6</b> &nbsp;&nbsp; "
            "Total Return: <b style='color:%7'>%8</b>"
            "</p>"
            "</body></html>")
            .arg(portfolioName.toHtmlEscaped(), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")),
                 totals.allPricesKnown ? QString() : tr(" &middot; some prices unavailable, entry price used"),
                 rowsHtml)
            .arg(QString::number(totals.cost, 'f', 2), QString::number(totals.value, 'f', 2), totalReturnColor,
                 totalReturnText);

    QTextDocument document;
    document.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setOutputFileName(filePath);
    document.print(&printer);

    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not write the PDF file."));
        return;
    }
}
