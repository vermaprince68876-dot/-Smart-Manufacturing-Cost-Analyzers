// Material - one raw material and its price per kilogram.
// Plain data only: no methods, no database code. Carried between layers.

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

struct Material {
    int         id;
    std::string name;
    double      costPerKg;
};

#endif // MATERIAL_H
