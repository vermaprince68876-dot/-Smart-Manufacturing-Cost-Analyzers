
#ifndef COST_REPORT_H
#define COST_REPORT_H

#include "Report.h"
#include "../database/DatabaseManager.h"

#include <string>

class CostReport : public Report {
public:
    explicit CostReport(DatabaseManager& db) : db_(db) {}

    void render(std::ostream& out) override;

    // Export all four report sections to Excel.
    bool exportExcel(const std::string& filename,
                     std::string& error);

private:
    void costSection(std::ostream& out);
    void efficiencySection(std::ostream& out);
    void volumeSection(std::ostream& out);
    void fitSection(std::ostream& out);

    DatabaseManager& db_;
};

#endif // COST_REPORT_H
