#include "SpreadFormulas.h"

namespace {

std::optional<double> findLegPrice(const QVector<Commodity> &commodities, const QVector<QString> &match)
{
    for (const QString &needle : match) {
        for (const Commodity &c : commodities) {
            if (c.name.toLower().contains(needle))
                return c.price;
        }
    }
    return std::nullopt;
}

} // namespace

const QVector<SpreadPreset> &spreadPresets()
{
    static const QVector<SpreadPreset> presets = {
        {
            QStringLiteral("wti-brent"),
            QStringLiteral("WTI – Brent"),
            QStringLiteral("Atlantic vs US Gulf benchmark differential. Negative = WTI discount."),
            {
                {{QStringLiteral("wti")}, 1.0},
                {{QStringLiteral("brent")}, -1.0},
            },
            QStringLiteral("$/bbl"),
        },
        {
            QStringLiteral("crack-321"),
            QStringLiteral("3:2:1 Crack Spread"),
            QStringLiteral("Refining margin: 3 barrels crude → 2 gasoline + 1 heating oil/diesel."),
            {
                {{QStringLiteral("gasoline"), QStringLiteral("rbob")}, 2.0 / 3.0 * 42.0},
                {{QStringLiteral("heating"), QStringLiteral("diesel"), QStringLiteral("ulsd")}, 1.0 / 3.0 * 42.0},
                {{QStringLiteral("wti")}, -1.0},
            },
            QStringLiteral("$/bbl"),
        },
        {
            QStringLiteral("crush"),
            QStringLiteral("Soybean Crush"),
            QStringLiteral("Soy processing margin: meal + oil revenue minus bean cost."),
            {
                {{QStringLiteral("soybean meal")}, 0.022},
                {{QStringLiteral("soybean oil")}, 11.0},
                {{QStringLiteral("soybean")}, -1.0},
            },
            QStringLiteral("$/bu"),
        },
        {
            QStringLiteral("gold-silver"),
            QStringLiteral("Gold / Silver Ratio"),
            QStringLiteral("Classic safe-haven ratio. Historical mean ~65."),
            {
                {{QStringLiteral("gold")}, 1.0},
                {{QStringLiteral("silver")}, -1.0}, // ratio handled specially below
            },
            QStringLiteral("ratio"),
        },
        {
            QStringLiteral("natgas-wti"),
            QStringLiteral("Nat Gas / WTI BTU Ratio"),
            QStringLiteral("Energy parity (gas in $/mmBtu vs WTI $/bbl ÷ 5.8)."),
            {
                {{QStringLiteral("natural gas"), QStringLiteral("henry hub")}, 1.0},
                {{QStringLiteral("wti")}, 1.0 / 5.8},
            },
            QStringLiteral("ratio"),
        },
    };
    return presets;
}

std::optional<double> computeSpread(const SpreadPreset &preset, const QVector<Commodity> &commodities)
{
    if (preset.id == QStringLiteral("gold-silver")) {
        const auto gold = findLegPrice(commodities, {QStringLiteral("gold")});
        const auto silver = findLegPrice(commodities, {QStringLiteral("silver")});
        if (!gold || !silver || *silver == 0.0)
            return std::nullopt;
        return *gold / *silver;
    }
    if (preset.id == QStringLiteral("natgas-wti")) {
        const auto gas = findLegPrice(commodities, {QStringLiteral("natural gas"), QStringLiteral("henry hub")});
        const auto wti = findLegPrice(commodities, {QStringLiteral("wti")});
        if (!gas || !wti || *wti == 0.0)
            return std::nullopt;
        return *gas / (*wti / 5.8);
    }

    double total = 0.0;
    for (const SpreadLeg &leg : preset.legs) {
        const auto price = findLegPrice(commodities, leg.match);
        if (!price)
            return std::nullopt;
        total += *price * leg.weight;
    }
    return total;
}
