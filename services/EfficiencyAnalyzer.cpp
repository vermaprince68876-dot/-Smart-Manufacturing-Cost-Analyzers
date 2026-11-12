#include "EfficiencyAnalyzer.h"

#include <cmath>

std::vector<MachineEfficiency> EfficiencyAnalyzer::byMachine(std::string& error) {
    std::vector<MachineEfficiency> out;
    const char* sql =
        "SELECT m.id, m.name, m.ideal_units_per_hour, "
        "       SUM(r.quantity_produced), SUM(r.quantity_rejected), SUM(r.machine_hours), "
        "       SUM(r.material_kg_used * mat.cost_per_kg "
        "           + r.machine_hours * m.hourly_rate + m.setup_cost_per_run) "
        "FROM production_runs r "
        "JOIN machines  m   ON m.id  = r.machine_id "
        "JOIN materials mat ON mat.id = r.material_id "
        "GROUP BY m.id, m.name, m.ideal_units_per_hour ORDER BY m.id;";
    sqlite3_stmt* stmt = db_.prepare(sql, error);
    if (!stmt) return out;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MachineEfficiency e;
        e.machineId     = sqlite3_column_int(stmt, 0);
        e.machineName   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        double ideal    = sqlite3_column_double(stmt, 2);
        e.totalProduced = sqlite3_column_int64(stmt, 3);
        e.totalRejected = sqlite3_column_int64(stmt, 4);
        e.totalHours    = sqlite3_column_double(stmt, 5);
        e.totalCost     = sqlite3_column_double(stmt, 6);
        e.totalGood     = e.totalProduced - e.totalRejected;

        e.rejectRatePercent  = e.totalProduced > 0
            ? 100.0 * e.totalRejected / e.totalProduced : 0.0;
        e.actualUnitsPerHour = e.totalHours > 0
            ? static_cast<double>(e.totalProduced) / e.totalHours : 0.0;
        e.efficiencyPercent  = ideal > 0
            ? 100.0 * e.actualUnitsPerHour / ideal : 0.0;
        e.avgCostPerUnit     = e.totalGood > 0
            ? e.totalCost / static_cast<double>(e.totalGood) : 0.0;
        out.push_back(e);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<VolumeBand> EfficiencyAnalyzer::byVolumeBand(std::string& error) {
    std::vector<VolumeBand> out;
    const char* sql =
        "SELECT CASE WHEN r.quantity_produced < 1000 THEN '< 1,000 units' "
        "            WHEN r.quantity_produced < 2500 THEN '1,000 - 2,499' "
        "            WHEN r.quantity_produced < 4000 THEN '2,500 - 3,999' "
        "            ELSE '>= 4,000 units' END AS band, "
        "       MIN(r.quantity_produced) AS sort_key, "
        "       SUM(r.quantity_produced - r.quantity_rejected) AS good, "
        "       SUM(r.material_kg_used * mat.cost_per_kg "
        "           + r.machine_hours * m.hourly_rate + m.setup_cost_per_run) AS cost "
        "FROM production_runs r "
        "JOIN machines  m   ON m.id  = r.machine_id "
        "JOIN materials mat ON mat.id = r.material_id "
        "GROUP BY band ORDER BY sort_key;";
    sqlite3_stmt* stmt = db_.prepare(sql, error);
    if (!stmt) return out;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VolumeBand b;
        b.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        b.totalUnits = sqlite3_column_int64(stmt, 2);
        double cost = sqlite3_column_double(stmt, 3);
        b.avgCostPerUnit = b.totalUnits > 0 ? cost / static_cast<double>(b.totalUnits) : 0.0;
        out.push_back(b);
    }
    sqlite3_finalize(stmt);
    return out;
}

Regression EfficiencyAnalyzer::costVolumeFit(std::string& error) {
    // Reuse the per-run cost via a single aggregate pass.
    const char* sql =
        "WITH run_cost AS ("
        "  SELECT r.quantity_produced AS volume, "
        "         (r.material_kg_used * mat.cost_per_kg + r.machine_hours * m.hourly_rate "
        "          + m.setup_cost_per_run) / (r.quantity_produced - r.quantity_rejected) AS cpu "
        "  FROM production_runs r "
        "  JOIN machines m ON m.id = r.machine_id "
        "  JOIN materials mat ON mat.id = r.material_id "
        "  WHERE r.quantity_produced - r.quantity_rejected > 0) "
        "SELECT COUNT(*), SUM(volume), SUM(cpu), SUM(volume*volume), "
        "       SUM(volume*cpu), SUM(cpu*cpu) FROM run_cost;";
    Regression r{};
    sqlite3_stmt* stmt = db_.prepare(sql, error);
    if (!stmt) return r;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        long n = sqlite3_column_int64(stmt, 0);
        double sx = sqlite3_column_double(stmt, 1);
        double sy = sqlite3_column_double(stmt, 2);
        double sxx = sqlite3_column_double(stmt, 3);
        double sxy = sqlite3_column_double(stmt, 4);
        double syy = sqlite3_column_double(stmt, 5);
        double denom = n * sxx - sx * sx;
        if (n >= 2 && denom != 0.0) {
            r.slope     = (n * sxy - sx * sy) / denom;
            r.intercept = (sy - r.slope * sx) / static_cast<double>(n);
            double num = n * sxy - sx * sy;
            double d2  = n * syy - sy * sy;
            r.r2 = (denom * d2) > 0 ? (num * num) / (denom * d2) : 0.0;
            r.n  = n;
        }
    }
    sqlite3_finalize(stmt);
    return r;
}
