#pragma once

#include <QString>
#include <QVector>
#include <optional>

#include "models/Commodity.h"

// Mirrors commodity-hub's src/utils/spreadFormulas.ts SPREAD_PRESETS/computeSpread —
// only the built-in presets, computed live from already-fetched Commodity prices.
// The web app's custom spread builder (saved to the `user_spreads` table, gated
// behind a premium paywall) has no desktop equivalent yet.

struct SpreadLeg
{
    QVector<QString> match; // lower-case substrings; first commodity whose name contains one wins
    double weight = 1.0; // positive = long, negative = short
};

struct SpreadPreset
{
    QString id;
    QString name;
    QString description;
    QVector<SpreadLeg> legs;
    QString unit;
};

const QVector<SpreadPreset> &spreadPresets();

// Returns nullopt if any leg's commodity isn't present in `commodities`.
std::optional<double> computeSpread(const SpreadPreset &preset, const QVector<Commodity> &commodities);
