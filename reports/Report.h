// Report - base class for anything that can be printed.
// Holds the money/number formatting so it is written once, not in every report.

#ifndef REPORT_H
#define REPORT_H

#include <ostream>
#include <sstream>
#include <string>

// Base class for anything that renders to a stream.
class Report {
public:
    virtual ~Report() = default;
    virtual void render(std::ostream& out) = 0;

protected:
    static std::string money(double v) {
        std::ostringstream oss;
        oss.setf(std::ios::fixed); oss.precision(2);
        oss << "INR " << v;
        return oss.str();
    }
    static std::string num(double v, int prec) {
        std::ostringstream oss;
        oss.setf(std::ios::fixed); oss.precision(prec);
        oss << v;
        return oss.str();
    }
};

#endif // REPORT_H
