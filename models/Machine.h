// Machine - one machine: what it costs to run per hour, how fast it is
// meant to go, and the fixed cost of setting it up for a batch.

#ifndef MACHINE_H
#define MACHINE_H

#include <string>

struct Machine {
    int         id;
    std::string name;
    double      hourlyRate;
    double      idealUnitsPerHour;
    double      setupCostPerRun;
};

#endif // MACHINE_H
