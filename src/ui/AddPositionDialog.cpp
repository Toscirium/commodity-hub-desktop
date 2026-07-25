#include "AddPositionDialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AddPositionDialog::AddPositionDialog(const QVector<Commodity> &commodities, QWidget *parent,
                                      const PortfolioPosition &existing)
    : QDialog(parent)
{
    const bool isEdit = !existing.id.isEmpty();
    setWindowTitle(isEdit ? tr("Edit Position") : tr("Add Position"));
    setMinimumWidth(320);

    m_commodityCombo = new QComboBox(this);
    for (const Commodity &c : commodities)
        m_commodityCombo->addItem(c.name, c.price);

    m_quantityEdit = new QLineEdit(this);
    m_quantityEdit->setValidator(new QDoubleValidator(0.0001, 1'000'000'000.0, 4, m_quantityEdit));
    m_quantityEdit->setPlaceholderText(tr("e.g. 10"));

    m_entryPriceEdit = new QLineEdit(this);
    m_entryPriceEdit->setValidator(new QDoubleValidator(0.0001, 1'000'000'000.0, 4, m_entryPriceEdit));

    m_entryDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_entryDateEdit->setCalendarPopup(true);
    m_entryDateEdit->setMaximumDate(QDate::currentDate());
    m_entryDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));

    m_notesEdit = new QLineEdit(this);
    m_notesEdit->setPlaceholderText(tr("Optional"));

    auto *form = new QFormLayout;
    form->addRow(tr("Commodity"), m_commodityCombo);
    form->addRow(tr("Quantity"), m_quantityEdit);
    form->addRow(tr("Entry Price"), m_entryPriceEdit);
    form->addRow(tr("Entry Date"), m_entryDateEdit);
    form->addRow(tr("Notes"), m_notesEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(isEdit ? tr("Save") : tr("Add"));
    connect(buttons, &QDialogButtonBox::accepted, this, &AddPositionDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_commodityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &AddPositionDialog::onCommodityChanged);

    if (isEdit) {
        const int index = m_commodityCombo->findText(existing.commodityName);
        if (index >= 0) {
            m_commodityCombo->setCurrentIndex(index);
        } else {
            // Commodity no longer in the live catalog (renamed/delisted); keep
            // the recorded name selectable so editing quantity/notes still works.
            m_commodityCombo->insertItem(0, existing.commodityName, existing.entryPrice);
            m_commodityCombo->setCurrentIndex(0);
        }
        m_quantityEdit->setText(QString::number(existing.quantity, 'f', 4));
        m_entryPriceEdit->setText(QString::number(existing.entryPrice, 'f', 2));
        const QDate date = QDate::fromString(existing.entryDate, Qt::ISODate);
        if (date.isValid())
            m_entryDateEdit->setDate(date);
        m_notesEdit->setText(existing.notes);
    } else if (m_commodityCombo->count() > 0) {
        onCommodityChanged(0);
    }
}

void AddPositionDialog::onCommodityChanged(int index)
{
    if (index < 0)
        return;
    // Pre-fill the entry price with the current market price as a convenience;
    // the user can still edit it to record their actual fill price.
    m_entryPriceEdit->setText(QString::number(m_commodityCombo->itemData(index).toDouble(), 'f', 2));
}

void AddPositionDialog::onAccept()
{
    if (m_commodityCombo->currentIndex() < 0 || m_quantityEdit->text().trimmed().isEmpty()
        || m_entryPriceEdit->text().trimmed().isEmpty())
        return;

    accept();
}

QString AddPositionDialog::commodityName() const
{
    return m_commodityCombo->currentText();
}

double AddPositionDialog::quantity() const
{
    return m_quantityEdit->text().toDouble();
}

double AddPositionDialog::entryPrice() const
{
    return m_entryPriceEdit->text().toDouble();
}

QString AddPositionDialog::entryDate() const
{
    return m_entryDateEdit->date().toString(Qt::ISODate);
}

QString AddPositionDialog::notes() const
{
    return m_notesEdit->text().trimmed();
}
