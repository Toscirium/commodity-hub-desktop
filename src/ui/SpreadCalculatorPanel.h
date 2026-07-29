#pragma once

#include <QVector>
#include <QWidget>

#include "models/Commodity.h"

class QListWidget;
class QLabel;

// Read-only "Tools" page computing commodity-hub.eu's built-in spread/ratio
// presets (WTI-Brent, crack spread, crush spread, ...) live from whatever
// prices MainWindow already fetched — no network calls of its own. See
// core/SpreadFormulas.h for the actual math; this class is purely the list
// view over it, refreshed whenever setCommodities() is called.
class SpreadCalculatorPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SpreadCalculatorPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);

private:
    void refreshSpreads();

    QVector<Commodity> m_commodities;
    QListWidget *m_list;
    QLabel *m_emptyLabel;
};
