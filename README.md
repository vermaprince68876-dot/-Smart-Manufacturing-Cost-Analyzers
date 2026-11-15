
# Smart Manufacturing Cost Analyzer

A small C++ + SQL program for a factory. It records production batches and tells
you what each good piece costs to make, which machine is cheapest, and how batch
size changes the cost.

Everything is included — SQLite is bundled — so it builds with **nothing
installed** except a C++ compiler.

---

## 1. Build and run (three commands)

```bash
make                    # builds ./costanalyzer
./costanalyzer init     # creates factory.db and fills it with sample data
./costanalyzer report   # prints the report
```

That is all. `make run` does the last step for you.

> On Windows, use `mingw32-make` instead of `make`, and run `costanalyzer.exe`.

---

## 2. What you should see

```
### 1. MANUFACTURING COST PER UNIT
Run  Date        Machine                  Good     Total Cost      Cost/Unit    Rej%
1    2026-01-08  CNC Milling M1            860   INR 51980.00      INR 60.44    4.4%
2    2026-01-15  Injection Moulder M2     2305   INR 75680.00      INR 32.83    4.0%
...
Portfolio average cost per good unit: INR 50.46

### 2. MACHINE-WISE PRODUCTION EFFICIENCY
Injection Moulder M2       23400     2.02%       319.2       88.7%       INR 28.65
...
### 3. EFFECT OF PRODUCTION VOLUME ON UNIT COST
< 1,000 units                   4656         INR 98.33
>= 4,000 units                 14220         INR 27.08

### 4. COST-VOLUME FIT (least squares)
Linear model:  unit_cost = 102.8135 + (-0.019089) * volume
R-squared = 0.6270   (n = 30 runs)
```

---

## 3. How the pieces fit together

Five layers, each one only talking to the layer below it. Read them in this
order and the project makes sense.

```
   main.cpp                 takes the command, wires everything together
      |
      v
   reports/                 turns numbers into text  (CostReport)
      |
      v
   services/                does the arithmetic      (CostAnalyzer,
      |                                               EfficiencyAnalyzer)
      v
   database/                talks to SQLite          (DatabaseManager)
      |
      v
   SQLite  (factory.db)     stores the rows
```

`models/` sits beside all of them: three plain structs (`Material`, `Machine`,
`ProductionRun`) that carry data between layers.

**One rule makes the whole thing readable:** only `reports/` prints. The services
return numbers, the database returns rows, and nothing else writes to the screen.

---

## 4. What happens when you run `report`

1. `main.cpp` opens `factory.db` (creating the file if it is missing).
2. `DatabaseManager::ensureSchema()` makes sure the three tables exist.
3. If the runs table is empty, it loads the sample factory.
4. `CostReport::render()` is called, which runs four sections in order:
   - `CostAnalyzer::costPerUnitByRun()` — cost per piece for every batch
   - `EfficiencyAnalyzer::byMachine()` — throughput and cost per machine
   - `EfficiencyAnalyzer::byVolumeBand()` — cost grouped by batch size
   - `EfficiencyAnalyzer::costVolumeFit()` — the straight line through the data
5. Each section prints its own table.

---

## 5. Folder map

```
SmartManufacturingCostAnalyzer/
├── Makefile                  one-command build
├── main.cpp                  entry point, command line, menu
├── models/                   plain data structs
│   ├── Material.h            id, name, cost per kg
│   ├── Machine.h             rate, ideal speed, setup cost
│   └── ProductionRun.h       one batch
├── services/                 the arithmetic
│   ├── CostAnalyzer.h/.cpp   cost per unit
│   └── EfficiencyAnalyzer.h/.cpp   efficiency, volume, fit
├── database/                 the SQL side
│   ├── DatabaseManager.h/.cpp      connection + schema + sample data
│   ├── schema.sql                  the tables, as a standalone script
│   ├── sqlite3.c                   bundled SQLite (no install needed)
│   └── sqlite3.h
├── reports/                  the output
│   ├── Report.h              base class + number formatting
│   └── CostReport.h/.cpp     the four printed sections
└── README.md
```

---

## 6. The formulas

For one batch:

```
material cost  = material_kg_used × cost_per_kg
machine cost   = machine_hours    × hourly_rate
total cost     = material + machine + setup_cost_per_run
good units     = produced − rejected
cost per unit  = total cost ÷ good units
```

Per machine:

```
reject rate = rejected ÷ produced × 100
actual rate = produced ÷ machine_hours
efficiency  = actual rate ÷ ideal_units_per_hour × 100
```

---

## 7. Commands

| Command | What it does |
|---|---|
| `make` | builds `./costanalyzer` |
| `make run` | builds, then prints the report |
| `make clean` | removes the build output and `factory.db` |
| `./costanalyzer init` | creates and fills `factory.db` |
| `./costanalyzer report` | prints the full report |
| `./costanalyzer menu` | interactive menu |
| `./costanalyzer report mydata.db` | uses a different database file |

---

## 8. The database

Three tables, created automatically on first run:

| Table | Holds |
|---|---|
| `materials` | each raw material and its cost per kg |
| `machines` | each machine's hourly rate, ideal speed, setup cost |
| `production_runs` | one row per batch |

You can also create it outside the program:

```bash
sqlite3 factory.db < database/schema.sql
```

---

## 9. If something goes wrong

| Problem | Fix |
|---|---|
| `make: command not found` | install `build-essential` (Linux) or use `mingw32-make` (Windows) |
| `g++: command not found` | install a C++ compiler |
| `database is locked` | close any SQLite browser that has `factory.db` open |
| Numbers look odd after editing data | run `./costanalyzer init` to reload the sample factory |


## 10.Wed link

https://smart-manufacturing-cost-analyzer.onrender.com/
