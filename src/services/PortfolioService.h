#pragma once

#include <QObject>
#include <QVector>

#include "models/Portfolio.h"
#include "models/PortfolioPosition.h"

class SupabaseClient;
class Session;

class PortfolioService : public QObject
{
    Q_OBJECT
public:
    PortfolioService(SupabaseClient &client, Session &session, QObject *parent = nullptr);

    void fetchPortfolios();
    void createPortfolio(const QString &name);

    // Empty portfolioId fetches all of the user's positions (matches the
    // free-tier "just one portfolio" case without the UI needing to know its id).
    void fetchPositions(const QString &portfolioId = QString());
    void addPosition(const QString &commodityName, double quantity, double entryPrice, const QString &entryDate,
                      const QString &notes, const QString &portfolioId = QString());
    void updatePosition(const QString &positionId, const QString &commodityName, double quantity,
                         double entryPrice, const QString &entryDate, const QString &notes);
    void removePosition(const QString &positionId);

signals:
    void portfoliosLoaded(const QVector<Portfolio> &portfolios);
    void positionsLoaded(const QVector<PortfolioPosition> &positions);
    void errorOccurred(const QString &message);

private:
    SupabaseClient &m_client;
    Session &m_session;
    QString m_currentPortfolioId;
};
