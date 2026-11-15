
#include "CostReport.h"
#include "../services/CostAnalyzer.h"
#include "../services/EfficiencyAnalyzer.h"

#include <xlsxwriter.h>

#include <iomanip>
#include <string>
#include <vector>

// Write a text cell to Excel.
static void writeText(lxw_worksheet* sheet,
                      lxw_row_t row,
                      lxw_col_t col,
                      const std::string& value,
                      lxw_format* format = nullptr) {
    worksheet_write_string(sheet, row, col, value.c_str(), format);
}

// Write a numeric cell to Excel.
static void writeNumber(lxw_worksheet* sheet,
                        lxw_row_t row,
                        lxw_col_t col,
                        double value,
                        lxw_format* format = nullptr) {
    worksheet_write_number(sheet, row, col, value, format);
}

// Create the Excel workbook containing all four report sections.
bool CostReport::exportExcel(const std::string& filename,
                             std::string& error) {
    error.clear();

    // Get actual data from the existing analyzers.
    std::string costError;
    std::string machineError;
    std::string volumeError;
    std::string regressionError;

    CostAnalyzer costAnalyzer(db_);
    EfficiencyAnalyzer efficiencyAnalyzer(db_);

    std::vector<CostBreakdown> costs =
        costAnalyzer.costPerUnitByRun(costError);

    std::vector<MachineEfficiency> machines =
        efficiencyAnalyzer.byMachine(machineError);

    std::vector<VolumeBand> bands =
        efficiencyAnalyzer.byVolumeBand(volumeError);

    Regression regression =
        efficiencyAnalyzer.costVolumeFit(regressionError);

    if (!costError.empty()) {
        error = costError;
        return false;
    }
    if (!machineError.empty()) {
        error = machineError;
        return false;
    }
    if (!volumeError.empty()) {
        error = volumeError;
        return false;
    }
    if (!regressionError.empty()) {
        error = regressionError;
        return false;
    }

    // Create the workbook.
    lxw_workbook* workbook = workbook_new(filename.c_str());

    if (!workbook) {
        error = "Could not create Excel workbook.";
        return false;
    }

    // Create four worksheets.
    lxw_worksheet* costSheet =
        workbook_add_worksheet(workbook, "Cost Analysis");

    lxw_worksheet* machineSheet =
        workbook_add_worksheet(workbook, "Machine Efficiency");

    lxw_worksheet* volumeSheet =
        workbook_add_worksheet(workbook, "Volume Analysis");

    lxw_worksheet* regressionSheet =
        workbook_add_worksheet(workbook, "Regression");

    if (!costSheet || !machineSheet ||
        !volumeSheet || !regressionSheet) {
        error = "Could not create Excel worksheets.";
        workbook_close(workbook);
        return false;
    }

    // Create bold heading format.
    lxw_format* headerFormat = workbook_add_format(workbook);
    format_set_bold(headerFormat);

    // ==========================================
    // SHEET 1: MANUFACTURING COST ANALYSIS
    // ==========================================

    const char* costHeaders[] = {
        "Run ID", "Date", "Machine", "Produced",
        "Rejected", "Good Units", "Material Cost",
        "Machine Cost", "Setup Cost", "Total Cost",
        "Cost Per Unit", "Rejection %"
    };

    for (int col = 0; col < 12; ++col) {
        worksheet_write_string(
            costSheet, 0, col, costHeaders[col], headerFormat
        );
    }

    double totalCost = 0.0;
    long totalGood = 0;

    for (size_t i = 0; i < costs.size(); ++i) {
        const CostBreakdown& c = costs[i];
        lxw_row_t row = static_cast<lxw_row_t>(i + 1);

        double rejectPercent = c.produced > 0
            ? 100.0 * c.rejected / c.produced : 0.0;

        writeNumber(costSheet, row, 0, c.runId);
        writeText(costSheet, row, 1, c.runDate);
        writeText(costSheet, row, 2, c.machineName);
        writeNumber(costSheet, row, 3, c.produced);
        writeNumber(costSheet, row, 4, c.rejected);
        writeNumber(costSheet, row, 5, c.goodUnits);
        writeNumber(costSheet, row, 6, c.materialCost);
        writeNumber(costSheet, row, 7, c.machineCost);
        writeNumber(costSheet, row, 8, c.setupCost);
        writeNumber(costSheet, row, 9, c.totalCost);
        writeNumber(costSheet, row, 10, c.costPerUnit);
        writeNumber(costSheet, row, 11, rejectPercent);

        totalCost += c.totalCost;
        totalGood += c.goodUnits;
    }

    lxw_row_t costSummaryRow =
        static_cast<lxw_row_t>(costs.size() + 2);

    writeText(costSheet, costSummaryRow, 0,
              "Portfolio Average Cost Per Good Unit", headerFormat);

    writeNumber(costSheet, costSummaryRow, 1,
                totalGood > 0 ? totalCost / totalGood : 0.0);

    worksheet_set_column(costSheet, 0, 11, 18, nullptr);

    // ==========================================
    // SHEET 2: MACHINE-WISE EFFICIENCY
    // ==========================================

    const char* machineHeaders[] = {
        "Machine ID", "Machine Name", "Produced",
        "Rejected", "Good Units", "Machine Hours",
        "Total Cost", "Reject Rate %", "Units Per Hour",
        "Efficiency %", "Average Cost Per Unit"
    };

    for (int col = 0; col < 11; ++col) {
        worksheet_write_string(
            machineSheet, 0, col, machineHeaders[col], headerFormat
        );
    }

    for (size_t i = 0; i < machines.size(); ++i) {
        const MachineEfficiency& m = machines[i];
        lxw_row_t row = static_cast<lxw_row_t>(i + 1);

        writeNumber(machineSheet, row, 0, m.machineId);
        writeText(machineSheet, row, 1, m.machineName);
        writeNumber(machineSheet, row, 2, m.totalProduced);
        writeNumber(machineSheet, row, 3, m.totalRejected);
        writeNumber(machineSheet, row, 4, m.totalGood);
        writeNumber(machineSheet, row, 5, m.totalHours);
        writeNumber(machineSheet, row, 6, m.totalCost);
        writeNumber(machineSheet, row, 7, m.rejectRatePercent);
        writeNumber(machineSheet, row, 8, m.actualUnitsPerHour);
        writeNumber(machineSheet, row, 9, m.efficiencyPercent);
        writeNumber(machineSheet, row, 10, m.avgCostPerUnit);
    }

    worksheet_set_column(machineSheet, 0, 10, 20, nullptr);

    // ==========================================
    // SHEET 3: PRODUCTION VOLUME ANALYSIS
    // ==========================================

    const char* volumeHeaders[] = {
        "Volume Band", "Good Units", "Average Cost Per Unit"
    };

    for (int col = 0; col < 3; ++col) {
        worksheet_write_string(
            volumeSheet, 0, col, volumeHeaders[col], headerFormat
        );
    }

    for (size_t i = 0; i < bands.size(); ++i) {
        const VolumeBand& b = bands[i];
        lxw_row_t row = static_cast<lxw_row_t>(i + 1);

        writeText(volumeSheet, row, 0, b.label);
        writeNumber(volumeSheet, row, 1, b.totalUnits);
        writeNumber(volumeSheet, row, 2, b.avgCostPerUnit);
    }

    worksheet_set_column(volumeSheet, 0, 2, 25, nullptr);

    // ==========================================
    // SHEET 4: COST-VOLUME REGRESSION
    // ==========================================

    writeText(regressionSheet, 0, 0, "Metric", headerFormat);
    writeText(regressionSheet, 0, 1, "Value", headerFormat);

    writeText(regressionSheet, 1, 0, "Intercept");
    writeNumber(regressionSheet, 1, 1, regression.intercept);

    writeText(regressionSheet, 2, 0, "Slope");
    writeNumber(regressionSheet, 2, 1, regression.slope);

    writeText(regressionSheet, 3, 0, "R-squared");
    writeNumber(regressionSheet, 3, 1, regression.r2);

    writeText(regressionSheet, 4, 0, "Number of Runs");
    writeNumber(regressionSheet, 4, 1, regression.n);

    writeText(regressionSheet, 6, 0, "Linear Model", headerFormat);
    writeText(regressionSheet, 6, 1, "Unit Cost = Intercept + Slope * Volume");

    worksheet_set_column(regressionSheet, 0, 1, 35, nullptr);

    // Finish and save the workbook.
    lxw_error result = workbook_close(workbook);

    if (result != LXW_NO_ERROR) {
        error = "Excel workbook could not be saved. Check the file path and permissions.";
        return false;
    }

    return true;
}

// ==========================================
// EXISTING CONSOLE REPORT
// ==========================================

void CostReport::render(std::ostream& out) {
    costSection(out);
    efficiencySection(out);
    volumeSection(out);
    fitSection(out);

    std::string err;
    out << "\nRows: runs=" << db_.countRows("production_runs", err)
        << "  machines=" << db_.countRows("machines", err)
        << "  materials=" << db_.countRows("materials", err) << "\n";

    // Automatically export the same report to Excel.
    std::string excelError;

    if (exportExcel("Manufacturing_Cost_Report.xlsx", excelError)) {
        out << "\n[OK] Excel report created: "
            << "Manufacturing_Cost_Report.xlsx\n";
    } else {
        out << "\n[ERROR] Excel export failed: "
            << excelError << "\n";
    }
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

    double total = 0.0;
    long good = 0;

    for (const auto& c : analyzer.costPerUnitByRun(err)) {
        double rej = c.produced > 0
            ? 100.0 * c.rejected / c.produced : 0.0;

        out << std::left << std::setw(5) << c.runId
            << std::setw(12) << c.runDate
            << std::setw(22) << c.machineName
            << std::right << std::setw(7) << c.goodUnits
            << std::setw(15) << money(c.totalCost)
            << std::setw(15) << money(c.costPerUnit)
            << std::setw(8) << (num(rej, 1) + "%") << "\n";

        total += c.totalCost;
        good += c.goodUnits;
    }

    out << std::string(84, '-') << "\n";
    out << "Portfolio average cost per good unit: "
        << money(good ? total / good : 0.0) << "\n";
}

void CostReport::efficiencySection(std::ostream& out) {
    std::string err;
    EfficiencyAnalyzer analyzer(db_);

    out << "\n### 2. MACHINE-WISE PRODUCTION EFFICIENCY\n";
    out << std::left << std::setw(22) << "Machine"
        << std::right << std::setw(10) << "Produced"
        << std::setw(10) << "Reject%"
        << std::setw(12) << "Units/hr"
        << std::setw(12) << "Efficiency"
        << std::setw(16) << "Avg Cost/Unit" << "\n";

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
    out << std::left << std::setw(22) << "Volume band"
        << std::right << std::setw(14) << "Good units"
        << std::setw(18) << "Avg cost / unit" << "\n";

    out << std::string(54, '-') << "\n";

    for (const auto& b : analyzer.byVolumeBand(err)) {
        out << std::left << std::setw(22) << b.label
            << std::right << std::setw(14) << b.totalUnits
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
    out << "R-squared = " << num(r.r2, 4)
        << "   (n = " << r.n << " runs)\n";
}
