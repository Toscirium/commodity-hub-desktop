#include "PortfolioService.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>

#include "core/Session.h"
#include "core/SupabaseClient.h"

PortfolioService::PortfolioService(SupabaseClient &client, Session &session, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_session(session)
{
}

void PortfolioService::fetchPortfolios()
{
    QPointer<PortfolioService> self(this);
    m_client.restGet(QStringLiteral("/rest/v1/portfolios?select=*&order=created_at.asc"),
                      [self](bool ok, const QJsonDocument &doc, const QString &error) {
                          if (!self)
                              return;

                          if (!ok) {
                              emit self->errorOccurred(error);
                              return;
                          }

                          QVector<Portfolio> portfolios;
                          const QJsonArray array = doc.array();
                          portfolios.reserve(array.size());
                          for (const QJsonValue &value : array)
                              portfolios.append(Portfolio::fromJson(value.toObject()));

                          emit self->portfoliosLoaded(portfolios);
                      });
}

void PortfolioService::createPortfolio(const QString &name)
{
    QJsonObject body;
    body["name"] = name;
    body["user_id"] = m_session.userId();

    QPointer<PortfolioService> self(this);
    m_client.restPost(QStringLiteral("/rest/v1/portfolios"), QJsonDocument(body),
                       [self](bool ok, const QJsonDocument &, const QString &error) {
                           if (!self)
                               return;
                           if (!ok) {
                               emit self->errorOccurred(error);
                               return;
                           }
                           self->fetchPortfolios();
                       });
}

void PortfolioService::fetchPositions(const QString &portfolioId)
{
    m_currentPortfolioId = portfolioId;

    QString path = QStringLiteral("/rest/v1/portfolio_positions?select=*&order=created_at.desc");
    if (!portfolioId.isEmpty())
        path += QStringLiteral("&portfolio_id=eq.") + QUrl::toPercentEncoding(portfolioId);

    QPointer<PortfolioService> self(this);
    m_client.restGet(path, [self](bool ok, const QJsonDocument &doc, const QString &error) {
        if (!self)
            return;

        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }

        QVector<PortfolioPosition> positions;
        const QJsonArray array = doc.array();
        positions.reserve(array.size());
        for (const QJsonValue &value : array)
            positions.append(PortfolioPosition::fromJson(value.toObject()));

        emit self->positionsLoaded(positions);
    });
}

void PortfolioService::addPosition(const QString &commodityName, double quantity, double entryPrice,
                                    const QString &entryDate, const QString &notes, const QString &portfolioId)
{
    QJsonObject body;
    body["commodity_name"] = commodityName;
    body["quantity"] = quantity;
    body["entry_price"] = entryPrice;
    body["entry_date"] = entryDate;
    body["user_id"] = m_session.userId();
    if (!notes.isEmpty())
        body["notes"] = notes;
    if (!portfolioId.isEmpty())
        body["portfolio_id"] = portfolioId;

    QPointer<PortfolioService> self(this);
    m_client.restPost(QStringLiteral("/rest/v1/portfolio_positions"), QJsonDocument(body),
                       [self](bool ok, const QJsonDocument &, const QString &error) {
                           if (!self)
                               return;
                           if (!ok) {
                               emit self->errorOccurred(error);
                               return;
                           }
                           self->fetchPositions(self->m_currentPortfolioId);
                       });
}

void PortfolioService::updatePosition(const QString &positionId, const QString &commodityName, double quantity,
                                       double entryPrice, const QString &entryDate, const QString &notes)
{
    QJsonObject body;
    body["commodity_name"] = commodityName;
    body["quantity"] = quantity;
    body["entry_price"] = entryPrice;
    body["entry_date"] = entryDate;
    body["notes"] = notes;

    const QString path = QStringLiteral("/rest/v1/portfolio_positions?id=eq.") + QUrl::toPercentEncoding(positionId);

    QPointer<PortfolioService> self(this);
    m_client.restPatch(path, QJsonDocument(body), [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        self->fetchPositions(self->m_currentPortfolioId);
    });
}

void PortfolioService::removePosition(const QString &positionId)
{
    const QString path = QStringLiteral("/rest/v1/portfolio_positions?id=eq.") + QUrl::toPercentEncoding(positionId);

    QPointer<PortfolioService> self(this);
    m_client.restDelete(path, [self](bool ok, const QJsonDocument &, const QString &error) {
        if (!self)
            return;
        if (!ok) {
            emit self->errorOccurred(error);
            return;
        }
        self->fetchPositions(self->m_currentPortfolioId);
    });
}
