#include "AddAlertDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

AddAlertDialog::AddAlertDialog(const QVector<Commodity> &commodities, QWidget *parent)
    : QDialog(parent)
    , m_commodities(commodities)
{
    setWindowTitle(tr("New Price Alert"));
    setMinimumWidth(320);

    m_commodityCombo = new QComboBox(this);
    for (const Commodity &c : commodities)
        m_commodityCombo->addItem(c.name, c.price);

    m_conditionCombo = new QComboBox(this);
    m_conditionCombo->addItem(tr("Goes above"), QStringLiteral("above"));
    m_conditionCombo->addItem(tr("Goes below"), QStringLiteral("below"));

    m_targetPriceEdit = new QLineEdit(this);
    m_targetPriceEdit->setValidator(new QDoubleValidator(0.0001, 1'000'000'000.0, 4, m_targetPriceEdit));

    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText(tr("Optional"));

    auto *form = new QFormLayout;
    form->addRow(tr("Commodity"), m_commodityCombo);
    form->addRow(tr("Condition"), m_conditionCombo);
    form->addRow(tr("Target Price"), m_targetPriceEdit);
    form->addRow(tr("Note"), m_noteEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &AddAlertDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_commodityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &AddAlertDialog::onCommodityChanged);

    if (m_commodityCombo->count() > 0)
        onCommodityChanged(0);
}

void AddAlertDialog::onCommodityChanged(int index)
{
    if (index < 0)
        return;
    // Pre-fill with the current price as a starting point for the threshold.
    m_targetPriceEdit->setText(QString::number(m_commodityCombo->itemData(index).toDouble(), 'f', 2));
}

void AddAlertDialog::onAccept()
{
    if (m_commodityCombo->currentIndex() < 0 || m_targetPriceEdit->text().trimmed().isEmpty())
        return;

    accept();
}

QString AddAlertDialog::commodityName() const
{
    return m_commodityCombo->currentText();
}

QString AddAlertDialog::commoditySymbol() const
{
    const int index = m_commodityCombo->currentIndex();
    if (index < 0 || index >= m_commodities.size())
        return QString();
    return m_commodities.at(index).symbol;
}

QString AddAlertDialog::condition() const
{
    return m_conditionCombo->currentData().toString();
}

double AddAlertDialog::targetPrice() const
{
    return m_targetPriceEdit->text().toDouble();
}

QString AddAlertDialog::note() const
{
    return m_noteEdit->text().trimmed();
}
