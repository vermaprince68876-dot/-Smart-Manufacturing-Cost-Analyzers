// EfficiencyAnalyzer - the three 'which machine, what batch size' analyses:
//   1. efficiency by machine
//   2. cost by production-volume band
//   3. a straight-line fit of unit cost against volume

#ifndef EFFICIENCY_ANALYZER_H
#define EFFICIENCY_ANALYZER_H

#include "../database/DatabaseManager.h"

#include <string>
#include <vector>

struct MachineEfficiency {
    int         machineId = 0;
    std::string machineName;
    long        totalProduced = 0;
    long        totalRejected = 0;
    long        totalGood = 0;
    double      totalHours = 0.0;
    double      totalCost = 0.0;
    double      rejectRatePercent = 0.0;
    double      actualUnitsPerHour = 0.0;
    double      efficiencyPercent = 0.0;
    double      avgCostPerUnit = 0.0;
};

struct VolumeBand {
    std::string label;
    long        totalUnits = 0;
    double      avgCostPerUnit = 0.0;
};

struct Regression {
    double slope = 0.0;
    double intercept = 0.0;
    double r2 = 0.0;
    long   n = 0;
};

// Machine efficiency, volume-versus-cost and the cost-volume fit.
class EfficiencyAnalyzer {
public:
    explicit EfficiencyAnalyzer(DatabaseManager& db) : db_(db) {}

    std::vector<MachineEfficiency> byMachine(std::string& error);
    std::vector<VolumeBand> byVolumeBand(std::string& error);
    Regression costVolumeFit(std::string& error);

private:
    DatabaseManager& db_;
};

#endif // EFFICIENCY_ANALYZER_H
