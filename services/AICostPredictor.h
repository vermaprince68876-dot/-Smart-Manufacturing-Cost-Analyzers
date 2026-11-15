
#ifndef AI_COST_PREDICTOR_H
#define AI_COST_PREDICTOR_H

#include "CostAnalyzer.h"

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

class AICostPredictor {
public:
    explicit AICostPredictor(DatabaseManager& db);

    bool train(std::string& error);

    void printOptions(std::ostream& out) const;
    void printModel(std::ostream& out) const;

    bool predict(int machineId,
                 int materialId,
                 double productionVolume,
                 double& predictedCost,
                 double& expectedGoodUnits,
                 bool& outsideHistoricalRange,
                 std::string& error) const;

private:
    struct ModelStats {
        double hoursPerUnit = 0.0;
        double kgPerUnit = 0.0;
        double rejectionRate = 0.0;
        double minVolume = 0.0;
        double maxVolume = 0.0;
        long samples = 0;
    };

    struct TrainingRecord {
        double produced = 0.0;
        double rejected = 0.0;
        double hours = 0.0;
        double kg = 0.0;
        double hourlyRate = 0.0;
        double setupCost = 0.0;
        double costPerKg = 0.0;
    };

    DatabaseManager& db_;

    std::map<std::pair<int, int>, ModelStats> models_;

    long totalSamples_ = 0;
    long validationSamples_ = 0;

    double mae_ = 0.0;
    double rmse_ = 0.0;

    bool trained_ = false;
};

#endif // AI_COST_PREDICTOR_H
