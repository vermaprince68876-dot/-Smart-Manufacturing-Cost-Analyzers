#include "CostAnalyzer.h"

std::vector<CostBreakdown> CostAnalyzer::costPerUnitByRun(std::string& error) {
    std::vector<CostBreakdown> rows;
    const char* sql =
        "SELECT r.id, r.run_date, m.name, r.quantity_produced, r.quantity_rejected, "
        "       r.machine_hours, r.material_kg_used, mat.cost_per_kg, "
        "       m.hourly_rate, m.setup_cost_per_run "
        "FROM production_runs r "
        "JOIN machines  m   ON m.id  = r.machine_id "
        "JOIN materials mat ON mat.id = r.material_id "
        "ORDER BY r.run_date, r.id;";
    sqlite3_stmt* stmt = db_.prepare(sql, error);
    if (!stmt) return rows;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CostBreakdown c;
        c.runId       = sqlite3_column_int64(stmt, 0);
        c.runDate     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.machineName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.produced    = sqlite3_column_int64(stmt, 3);
        c.rejected    = sqlite3_column_int64(stmt, 4);
        double hours  = sqlite3_column_double(stmt, 5);
        double kg     = sqlite3_column_double(stmt, 6);
        double ckg    = sqlite3_column_double(stmt, 7);
        double rate   = sqlite3_column_double(stmt, 8);
        double setup  = sqlite3_column_double(stmt, 9);

        c.goodUnits    = c.produced - c.rejected;
        c.materialCost = kg * ckg;
        c.machineCost  = hours * rate;
        c.setupCost    = setup;
        c.totalCost    = c.materialCost + c.machineCost + c.setupCost;
        c.costPerUnit  = c.goodUnits > 0 ? c.totalCost / static_cast<double>(c.goodUnits) : 0.0;
        rows.push_back(c);
    }
    sqlite3_finalize(stmt);
    return rows;
}

double CostAnalyzer::portfolioAverageCostPerUnit(std::string& error) {
    auto rows = costPerUnitByRun(error);
    double total = 0.0;
    long good = 0;
    for (const auto& c : rows) { total += c.totalCost; good += c.goodUnits; }
    return good > 0 ? total / static_cast<double>(good) : 0.0;
}
