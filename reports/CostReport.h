// CostReport - prints the four report sections, in order, to any stream.

#ifndef COST_REPORT_H
#define COST_REPORT_H

#include "Report.h"
#include "../database/DatabaseManager.h"

class CostReport : public Report {
public:
    explicit CostReport(DatabaseManager& db) : db_(db) {}
    void render(std::ostream& out) override;

private:
    void costSection(std::ostream& out);
    void efficiencySection(std::ostream& out);
    void volumeSection(std::ostream& out);
    void fitSection(std::ostream& out);

    DatabaseManager& db_;
};

#endif // COST_REPORT_H
