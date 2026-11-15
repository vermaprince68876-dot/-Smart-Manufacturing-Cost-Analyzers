
#include "database/DatabaseManager.h"
#include "reports/CostReport.h"
#include "services/AICostPredictor.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

const char* kDefaultDb = "factory.db";

void banner() {
    std::cout <<
        "============================================================\n"
        "  Smart Manufacturing Cost Analyzer\n"
        "  C++ + SQL + AI\n"
        "============================================================\n";
}

void menu(DatabaseManager& db) {

    for (;;) {

        std::cout << "\n===== Menu =====\n"
                  << "  1) Full report\n"
                  << "  2) Re-seed sample data\n"
                  << "  3) AI Cost Prediction\n"
                  << "  0) Exit\n"
                  << "Select: ";

        int c = -1;

        if (!(std::cin >> c)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            std::cout << "[ERROR] Enter a valid menu number.\n";
            continue;
        }

        std::string err;

        if (c == 0) {
            return;
        }

        if (c == 1) {
            CostReport(db).render(std::cout);
        }

        else if (c == 2) {

            std::cout
                << "[WARNING] This will replace existing sample records.\n"
                << "Continue? (1 = Yes, 0 = No): ";

            int confirm = 0;

            if (!(std::cin >> confirm)) {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                std::cout << "[ERROR] Invalid input.\n";
                continue;
            }

            if (confirm == 1) {
                if (db.seedSampleData(err)) {
                    std::cout << "[OK] Sample data reloaded.\n";
                }
                else {
                    std::cerr << "[ERROR] " << err << "\n";
                }
            }
        }

        else if (c == 3) {

            AICostPredictor ai(db);

            std::cout << "\nTraining AI model using historical data...\n";

            err.clear();

            if (!ai.train(err)) {
    std::cerr << "[ERROR] " << err << "\n";
    continue;
}



            ai.printModel(std::cout);
            ai.printOptions(std::cout);

            int machineId = 0;
            int materialId = 0;
            double volume = 0.0;

            std::cout << "\nEnter machine ID: ";

            if (!(std::cin >> machineId)) {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                std::cout << "[ERROR] Invalid machine ID.\n";
                continue;
            }

            std::cout << "Enter material ID: ";

            if (!(std::cin >> materialId)) {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                std::cout << "[ERROR] Invalid material ID.\n";
                continue;
            }

            std::cout << "Enter planned production quantity: ";

            if (!(std::cin >> volume)) {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );
                std::cout << "[ERROR] Invalid production quantity.\n";
                continue;
            }

            double predictedCost = 0.0;
            double expectedGoodUnits = 0.0;
            bool outsideHistoricalRange = false;

            err.clear();

            if (ai.predict(
                    machineId,
                    materialId,
                    volume,
                    predictedCost,
                    expectedGoodUnits,
                    outsideHistoricalRange,
                    err
                )) {

                std::cout << std::fixed << std::setprecision(2);

                std::cout << "\n========== AI COST PREDICTION ==========\n";

                std::cout << "Machine ID: "
                          << machineId << "\n";

                std::cout << "Material ID: "
                          << materialId << "\n";

                std::cout << "Planned production: "
                          << volume << " units\n";

                std::cout << "Expected good units: "
                          << expectedGoodUnits << "\n";

                std::cout << "Predicted cost per good unit: Rs. "
                          << predictedCost << "\n";
                if (outsideHistoricalRange) {
    std::cout << "\n[WARNING] Production quantity is outside "
              << "the historical training range.\n";

    std::cout << "This prediction may be less reliable.\n";
}          

                std::cout << "========================================\n";

                std::cout
                    << "Note: This is an estimate based on historical data.\n";
            }
            else {
                std::cerr << "[ERROR] " << err << "\n";
            }
        }

        else {
            std::cout << "[ERROR] Invalid menu option.\n";
        }
    }
}

} // namespace

int main(int argc, char** argv) {

    banner();

    std::string dbPath =
        (argc > 2) ? argv[2] : kDefaultDb;

    std::string cmd =
        (argc > 1) ? argv[1] : "report";

    DatabaseManager db;
    std::string err;

    if (!db.open(dbPath, err)) {
        std::cerr << "[ERROR] " << err << "\n";
        return 1;
    }

    if (!db.ensureSchema(err)) {
        std::cerr << "[ERROR] " << err << "\n";
        return 1;
    }

    if (cmd == "init" || cmd == "seed") {

        if (!db.seedSampleData(err)) {
            std::cerr << "[ERROR] " << err << "\n";
            return 1;
        }

        std::cout << "[OK] Database ready at " << dbPath << "\n";
        return 0;
    }

    if (cmd == "report") {

        if (db.countRows("production_runs", err) == 0) {
            if (!db.seedSampleData(err)) {
                std::cerr << "[ERROR] " << err << "\n";
                return 1;
            }
        }

        CostReport(db).render(std::cout);
        return 0;
    }

    if (cmd == "menu") {
        menu(db);
        return 0;
    }

    std::cout << "Usage: " << argv[0]
              << " [init|seed|report|menu] [db-file]\n";

    return 1;
}
