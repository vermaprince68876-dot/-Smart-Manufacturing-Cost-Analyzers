// ProductionRun - one manufacturing batch.
// This is the fact the whole project is built on: what was made, on which
// machine, from which material, how long it took, how much was scrapped.

#ifndef PRODUCTION_RUN_H
#define PRODUCTION_RUN_H

#include <string>

struct ProductionRun {
    int         id;
    std::string runDate;
    int         machineId;
    int         materialId;
    long        quantityProduced;
    long        quantityRejected;
    double      machineHours;
    double      materialKgUsed;
};

#endif // PRODUCTION_RUN_H
