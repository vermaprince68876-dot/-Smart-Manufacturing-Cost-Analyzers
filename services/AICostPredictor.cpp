
#include "AICostPredictor.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>

AICostPredictor::AICostPredictor(DatabaseManager& db)
    : db_(db) {
}

bool AICostPredictor::train(std::string& error) {

    error.clear();
    models_.clear();
    totalSamples_ = 0;
    validationSamples_ = 0;
    mae_ = 0.0;
    rmse_ = 0.0;
    trained_ = false;

    const char* sql =
        "SELECT r.machine_id, r.material_id, "
        "r.quantity_produced, r.quantity_rejected, "
        "r.machine_hours, r.material_kg_used, "
        "m.hourly_rate, m.setup_cost_per_run, "
        "mat.cost_per_kg "
        "FROM production_runs r "
        "JOIN machines m ON m.id = r.machine_id "
        "JOIN materials mat ON mat.id = r.material_id "
        "ORDER BY r.machine_id, r.material_id, r.id;";

    sqlite3_stmt* stmt = db_.prepare(sql, error);

    if (!stmt) {
        return false;
    }

    std::map<std::pair<int, int>, std::vector<TrainingRecord>> groups;

    int result = SQLITE_ROW;

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {

        int machineId = sqlite3_column_int(stmt, 0);
        int materialId = sqlite3_column_int(stmt, 1);

        TrainingRecord record;

        record.produced = sqlite3_column_double(stmt, 2);
        record.rejected = sqlite3_column_double(stmt, 3);
        record.hours = sqlite3_column_double(stmt, 4);
        record.kg = sqlite3_column_double(stmt, 5);
        record.hourlyRate = sqlite3_column_double(stmt, 6);
        record.setupCost = sqlite3_column_double(stmt, 7);
        record.costPerKg = sqlite3_column_double(stmt, 8);

        if (record.produced <= 0.0 ||
            record.rejected < 0.0 ||
            record.rejected >= record.produced ||
            record.hours <= 0.0 ||
            record.kg <= 0.0) {
            continue;
        }

        groups[{machineId, materialId}].push_back(record);
        ++totalSamples_;
    }

    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE) {
        error = db_.lastError();
        return false;
    }

    if (totalSamples_ < 2) {
        error = "Not enough valid production records for training.";
        return false;
    }

    double absoluteErrorSum = 0.0;
    double squaredErrorSum = 0.0;

    // Train each machine/material combination.
    for (const auto& group : groups) {

        const auto& records = group.second;

        double sumProduced = 0.0;
        double sumRejected = 0.0;
        double sumHours = 0.0;
        double sumKg = 0.0;

        double minVolume = records.front().produced;
        double maxVolume = records.front().produced;

        for (const auto& record : records) {

            sumProduced += record.produced;
            sumRejected += record.rejected;
            sumHours += record.hours;
            sumKg += record.kg;

            minVolume = std::min(minVolume, record.produced);
            maxVolume = std::max(maxVolume, record.produced);
        }

        ModelStats stats;

        stats.hoursPerUnit = sumHours / sumProduced;
        stats.kgPerUnit = sumKg / sumProduced;
        stats.rejectionRate = sumRejected / sumProduced;
        stats.minVolume = minVolume;
        stats.maxVolume = maxVolume;
        stats.samples = static_cast<long>(records.size());

        models_[group.first] = stats;

        // Leave-one-out validation:
        // predict each run using all other runs in its group.
        if (records.size() < 2) {
            continue;
        }

        for (const auto& actual : records) {

            double otherProduced = sumProduced - actual.produced;
            double otherRejected = sumRejected - actual.rejected;
            double otherHours = sumHours - actual.hours;
            double otherKg = sumKg - actual.kg;

            if (otherProduced <= 0.0) {
                continue;
            }

            double hoursPerUnit = otherHours / otherProduced;
            double kgPerUnit = otherKg / otherProduced;
            double rejectionRate = otherRejected / otherProduced;

            double expectedGood =
                actual.produced * (1.0 - rejectionRate);

            if (expectedGood <= 0.0) {
                continue;
            }

            double estimatedHours =
                hoursPerUnit * actual.produced;

            double estimatedKg =
                kgPerUnit * actual.produced;

            double estimatedTotalCost =
                estimatedHours * actual.hourlyRate +
                estimatedKg * actual.costPerKg +
                actual.setupCost;

            double predicted =
                estimatedTotalCost / expectedGood;

            double actualGood =
                actual.produced - actual.rejected;

            double actualTotalCost =
                actual.hours * actual.hourlyRate +
                actual.kg * actual.costPerKg +
                actual.setupCost;

            double actualCostPerUnit =
                actualTotalCost / actualGood;

            if (!std::isfinite(predicted) ||
                !std::isfinite(actualCostPerUnit)) {
                continue;
            }

            double difference = predicted - actualCostPerUnit;

            absoluteErrorSum += std::abs(difference);
            squaredErrorSum += difference * difference;

            ++validationSamples_;
        }
    }

    if (validationSamples_ == 0) {
        error = "Not enough grouped records for validation.";
        return false;
    }

    mae_ = absoluteErrorSum / validationSamples_;
    rmse_ = std::sqrt(squaredErrorSum / validationSamples_);

    trained_ = true;
    return true;
}

void AICostPredictor::printOptions(std::ostream& out) const {

    out << "\n========== AVAILABLE MACHINES ==========\n";

    std::string error;

    sqlite3_stmt* stmt = db_.prepare(
        "SELECT id, name FROM machines ORDER BY id;",
        error
    );

    if (!stmt) {
        out << "Could not load machines: " << error << "\n";
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        int id = sqlite3_column_int(stmt, 0);

        const unsigned char* rawName =
            sqlite3_column_text(stmt, 1);

        const char* name = rawName
            ? reinterpret_cast<const char*>(rawName)
            : "Unknown";

        out << "  " << id << ") " << name << "\n";
    }

    sqlite3_finalize(stmt);

    out << "\n========== AVAILABLE MATERIALS ==========\n";

    error.clear();

    stmt = db_.prepare(
        "SELECT id, name FROM materials ORDER BY id;",
        error
    );

    if (!stmt) {
        out << "Could not load materials: " << error << "\n";
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        int id = sqlite3_column_int(stmt, 0);

        const unsigned char* rawName =
            sqlite3_column_text(stmt, 1);

        const char* name = rawName
            ? reinterpret_cast<const char*>(rawName)
            : "Unknown";

        out << "  " << id << ") " << name << "\n";
    }

    sqlite3_finalize(stmt);
}

void AICostPredictor::printModel(std::ostream& out) const {

    out << "\n========================================\n";
    out << "     AI MANUFACTURING COST MODEL\n";
    out << "========================================\n";

    if (!trained_) {
        out << "AI model is not trained.\n";
        return;
    }

    out << std::fixed << std::setprecision(2);

    out << "Training records : " << totalSamples_ << "\n";
    out << "Validation predictions : " << validationSamples_ << "\n";

    out << "\nLeave-one-out validation results:\n";
    out << "MAE  : Rs. " << mae_ << " per good unit\n";
    out << "RMSE : Rs. " << rmse_ << " per good unit\n";

    out << "\nMAE is the average absolute prediction error.\n";
    out << "RMSE penalizes larger prediction errors more heavily.\n";

    out << "\n========================================\n";
}

bool AICostPredictor::predict(
    int machineId,
    int materialId,
    double productionVolume,
    double& predictedCost,
    double& expectedGoodUnits,
    bool& outsideHistoricalRange,
    std::string& error
) const {

    error.clear();
    outsideHistoricalRange = false;

    if (!trained_) {
        error = "AI model has not been trained.";
        return false;
    }

    if (machineId <= 0 || materialId <= 0) {
        error = "Invalid machine or material ID.";
        return false;
    }

    if (!std::isfinite(productionVolume) ||
        productionVolume <= 0.0) {
        error = "Production quantity must be positive.";
        return false;
    }

    auto modelIt = models_.find({machineId, materialId});

    if (modelIt == models_.end()) {
        error =
            "No historical data exists for this machine/material combination.";
        return false;
    }

    const ModelStats& stats = modelIt->second;

    outsideHistoricalRange =
        productionVolume < stats.minVolume ||
        productionVolume > stats.maxVolume;

    sqlite3_stmt* machineStmt = db_.prepare(
        "SELECT hourly_rate, setup_cost_per_run "
        "FROM machines WHERE id = ?;",
        error
    );

    if (!machineStmt) {
        return false;
    }

    sqlite3_bind_int(machineStmt, 1, machineId);

    if (sqlite3_step(machineStmt) != SQLITE_ROW) {
        sqlite3_finalize(machineStmt);
        error = "Selected machine was not found.";
        return false;
    }

    double hourlyRate =
        sqlite3_column_double(machineStmt, 0);

    double setupCost =
        sqlite3_column_double(machineStmt, 1);

    sqlite3_finalize(machineStmt);

    sqlite3_stmt* materialStmt = db_.prepare(
        "SELECT cost_per_kg FROM materials WHERE id = ?;",
        error
    );

    if (!materialStmt) {
        return false;
    }

    sqlite3_bind_int(materialStmt, 1, materialId);

    if (sqlite3_step(materialStmt) != SQLITE_ROW) {
        sqlite3_finalize(materialStmt);
        error = "Selected material was not found.";
        return false;
    }

    double costPerKg =
        sqlite3_column_double(materialStmt, 0);

    sqlite3_finalize(materialStmt);

    double estimatedHours =
        stats.hoursPerUnit * productionVolume;

    double estimatedMaterialKg =
        stats.kgPerUnit * productionVolume;

    expectedGoodUnits =
        productionVolume * (1.0 - stats.rejectionRate);

    if (expectedGoodUnits <= 0.0) {
        error = "Estimated good production quantity is zero.";
        return false;
    }

    double estimatedMachineCost =
        estimatedHours * hourlyRate;

    double estimatedMaterialCost =
        estimatedMaterialKg * costPerKg;

    double estimatedTotalCost =
        estimatedMachineCost +
        estimatedMaterialCost +
        setupCost;

    predictedCost =
        estimatedTotalCost / expectedGoodUnits;

    if (!std::isfinite(predictedCost) ||
        predictedCost < 0.0) {
        error = "AI prediction produced an invalid cost.";
        return false;
    }

    return true;
}
