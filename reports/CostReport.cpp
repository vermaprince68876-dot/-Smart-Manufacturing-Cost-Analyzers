#include "CostReport.h"
#include "../services/CostAnalyzer.h"
#include "../services/EfficiencyAnalyzer.h"

#include <iomanip>

void CostReport::render(std::ostream& out) {
    costSection(out);
    efficiencySection(out);
    volumeSection(out);
    fitSection(out);

    std::string err;
    out << "\nRows: runs=" << db_.countRows("production_runs", err)
        << "  machines=" << db_.countRows("machines", err)
        << "  materials=" << db_.countRows("materials", err) << "\n";
}

void CostReport::costSection(std::ostream& out) {
    std::string err;
    CostAnalyzer analyzer(db_);

    out << "\n### 1. MANUFACTURING COST PER UNIT\n";
    out << std::left << std::setw(5) << "Run" << std::setw(12) << "Date"
        << std::setw(22) << "Machine" << std::right << std::setw(7) << "Good"
        << std::setw(15) << "Total Cost" << std::setw(15) << "Cost/Unit"
        << std::setw(8) << "Rej%" << "\n";
    out << std::string(84, '-') << "\n";

    double total = 0.0; long good = 0;
    for (const auto& c : analyzer.costPerUnitByRun(err)) {
        double rej = c.produced > 0 ? 100.0 * c.rejected / c.produced : 0.0;
        out << std::left << std::setw(5) << c.runId << std::setw(12) << c.runDate
            << std::setw(22) << c.machineName << std::right << std::setw(7) << c.goodUnits
            << std::setw(15) << money(c.totalCost) << std::setw(15) << money(c.costPerUnit)
            << std::setw(8) << (num(rej, 1) + "%") << "\n";
        total += c.totalCost; good += c.goodUnits;
    }
    out << std::string(84, '-') << "\n";
    out << "Portfolio average cost per good unit: "
        << money(good ? total / good : 0.0) << "\n";
}

void CostReport::efficiencySection(std::ostream& out) {
    std::string err;
    EfficiencyAnalyzer analyzer(db_);

    out << "\n### 2. MACHINE-WISE PRODUCTION EFFICIENCY\n";
    out << std::left << std::setw(22) << "Machine" << std::right << std::setw(10) << "Produced"
        << std::setw(10) << "Reject%" << std::setw(12) << "Units/hr"
        << std::setw(12) << "Efficiency" << std::setw(16) << "Avg Cost/Unit" << "\n";
    out << std::string(82, '-') << "\n";
    for (const auto& e : analyzer.byMachine(err)) {
        out << std::left << std::setw(22) << e.machineName
            << std::right << std::setw(10) << e.totalProduced
            << std::setw(10) << (num(e.rejectRatePercent, 2) + "%")
            << std::setw(12) << num(e.actualUnitsPerHour, 1)
            << std::setw(12) << (num(e.efficiencyPercent, 1) + "%")
            << std::setw(16) << money(e.avgCostPerUnit) << "\n";
    }
}

void CostReport::volumeSection(std::ostream& out) {
    std::string err;
    EfficiencyAnalyzer analyzer(db_);

    out << "\n### 3. EFFECT OF PRODUCTION VOLUME ON UNIT COST\n";
    out << std::left << std::setw(22) << "Volume band" << std::right << std::setw(14) << "Good units"
        << std::setw(18) << "Avg cost / unit" << "\n";
    out << std::string(54, '-') << "\n";
    for (const auto& b : analyzer.byVolumeBand(err)) {
        out << std::left << std::setw(22) << b.label << std::right << std::setw(14) << b.totalUnits
            << std::setw(18) << money(b.avgCostPerUnit) << "\n";
    }
}

void CostReport::fitSection(std::ostream& out) {
    std::string err;
    EfficiencyAnalyzer analyzer(db_);
    Regression r = analyzer.costVolumeFit(err);

    out << "\n### 4. COST-VOLUME FIT (least squares)\n";
    out << "Linear model:  unit_cost = " << num(r.intercept, 4)
        << " + (" << num(r.slope, 6) << ") * volume\n";
    out << "R-squared = " << num(r.r2, 4) << "   (n = " << r.n << " runs)\n";
}
