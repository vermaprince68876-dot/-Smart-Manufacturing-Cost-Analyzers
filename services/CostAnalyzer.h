// CostAnalyzer - works out what one good unit costs to make.
// Returns plain structs; it never prints anything.

#ifndef COST_ANALYZER_H
#define COST_ANALYZER_H

#include "../database/DatabaseManager.h"

#include <string>
#include <vector>

// Cost of one production run, split into its three components.
struct CostBreakdown {
    long        runId = 0;
    std::string runDate;
    std::string machineName;
    long        produced = 0;
    long        rejected = 0;
    long        goodUnits = 0;
    double      materialCost = 0.0;
    double      machineCost = 0.0;
    double      setupCost = 0.0;
    double      totalCost = 0.0;
    double      costPerUnit = 0.0;
};

// Computes manufacturing cost per good unit.
class CostAnalyzer {
public:
    explicit CostAnalyzer(DatabaseManager& db) : db_(db) {}

    std::vector<CostBreakdown> costPerUnitByRun(std::string& error);
    double portfolioAverageCostPerUnit(std::string& error);

private:
    DatabaseManager& db_;
};

#endif // COST_ANALYZER_H
