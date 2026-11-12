// main.cpp - entry point and command line.
//
// The flow is short and always the same:
//
//     open the database  ->  make sure the tables exist  ->  run one command
//
//   init / seed   create the sample factory and stop
//   report        print the full report (seeding first if the database is empty)
//   menu          ask what to do, repeatedly
//
// Nothing here does any arithmetic: it wires the pieces together and prints.

#include "database/DatabaseManager.h"
#include "reports/CostReport.h"

#include <iostream>
#include <string>

namespace {

const char* kDefaultDb = "factory.db";

void banner() {
    std::cout <<
        "============================================================\n"
        "  Smart Manufacturing Cost Analyzer\n"
        "  C++ + SQL\n"
        "============================================================\n";
}

void menu(DatabaseManager& db) {
    for (;;) {
        std::cout << "\n===== Menu =====\n"
                  << "  1) Full report\n"
                  << "  2) Re-seed sample data\n"
                  << "  0) Exit\nSelect: ";
        int c = -1;
        if (!(std::cin >> c)) return;
        std::string err;
        if (c == 0) return;
        if (c == 1) { CostReport(db).render(std::cout); }
        else if (c == 2) {
            if (db.seedSampleData(err)) std::cout << "[OK] Sample data reloaded.\n";
            else std::cerr << "[ERROR] " << err << "\n";
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    banner();
    std::string dbPath = (argc > 2) ? argv[2] : kDefaultDb;
    std::string cmd = (argc > 1) ? argv[1] : "report";

    DatabaseManager db;
    std::string err;
    if (!db.open(dbPath, err)) { std::cerr << "[ERROR] " << err << "\n"; return 1; }
    if (!db.ensureSchema(err)) { std::cerr << "[ERROR] " << err << "\n"; return 1; }

    if (cmd == "init" || cmd == "seed") {
        if (!db.seedSampleData(err)) { std::cerr << "[ERROR] " << err << "\n"; return 1; }
        std::cout << "[OK] Database ready at " << dbPath << "\n";
        return 0;
    }
    if (cmd == "report") {
        if (db.countRows("production_runs", err) == 0) db.seedSampleData(err);
        CostReport(db).render(std::cout);
        return 0;
    }
    if (cmd == "menu") { menu(db); return 0; }

    std::cout << "Usage: " << argv[0] << " [init|seed|report|menu] [db-file]\n";
    return 1;
}
