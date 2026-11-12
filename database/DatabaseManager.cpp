#include "DatabaseManager.h"

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::open(const std::string& path, std::string& error) {
    close();
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        error = "Cannot open database: ";
        error += db_ ? sqlite3_errmsg(db_) : "unknown error";
        close();
        return false;
    }
    execute("PRAGMA foreign_keys = ON;", error);
    return true;
}

void DatabaseManager::close() {
    if (db_) { sqlite3_close(db_); db_ = nullptr; }
}

bool DatabaseManager::execute(const std::string& sql, std::string& error) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error = errMsg ? errMsg : "SQL execution failed";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3_stmt* DatabaseManager::prepare(const std::string& sql, std::string& error) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        error = "Prepare failed: ";
        error += sqlite3_errmsg(db_);
        return nullptr;
    }
    return stmt;
}

long DatabaseManager::countRows(const std::string& table, std::string& error) {
    sqlite3_stmt* stmt = prepare("SELECT COUNT(*) FROM " + table + ";", error);
    if (!stmt) return -1;
    long n = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

std::string DatabaseManager::lastError() const {
    return db_ ? sqlite3_errmsg(db_) : "no database";
}

bool DatabaseManager::ensureSchema(std::string& error) {
    // Same statements as database/schema.sql, which is the reference copy.
    const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS materials (
    id           INTEGER PRIMARY KEY,
    name         TEXT    NOT NULL,
    cost_per_kg  REAL    NOT NULL CHECK (cost_per_kg >= 0)
);
CREATE TABLE IF NOT EXISTS machines (
    id                   INTEGER PRIMARY KEY,
    name                 TEXT    NOT NULL,
    hourly_rate          REAL    NOT NULL CHECK (hourly_rate >= 0),
    ideal_units_per_hour REAL    NOT NULL CHECK (ideal_units_per_hour > 0),
    setup_cost_per_run   REAL    NOT NULL CHECK (setup_cost_per_run >= 0)
);
CREATE TABLE IF NOT EXISTS production_runs (
    id                 INTEGER PRIMARY KEY AUTOINCREMENT,
    run_date           TEXT    NOT NULL,
    machine_id         INTEGER NOT NULL REFERENCES machines(id),
    material_id        INTEGER NOT NULL REFERENCES materials(id),
    quantity_produced  INTEGER NOT NULL CHECK (quantity_produced >= 0),
    quantity_rejected  INTEGER NOT NULL CHECK (quantity_rejected >= 0),
    machine_hours      REAL    NOT NULL CHECK (machine_hours > 0),
    material_kg_used   REAL    NOT NULL CHECK (material_kg_used > 0),
    CHECK (quantity_rejected <= quantity_produced)
);
CREATE INDEX IF NOT EXISTS idx_runs_machine ON production_runs(machine_id);
CREATE INDEX IF NOT EXISTS idx_runs_date    ON production_runs(run_date);
)SQL";
    return execute(schema, error);
}

bool DatabaseManager::seedSampleData(std::string& error) {
    const char* seed = R"SQL(
DELETE FROM production_runs;
DELETE FROM machines;
DELETE FROM materials;
INSERT INTO materials (id, name, cost_per_kg) VALUES
    (1,'Mild Steel Plate',72.50),(2,'Aluminium Alloy 6061',210.00),
    (3,'Copper Wire 2mm',640.00),(4,'ABS Polymer Granules',135.25),
    (5,'Stainless Steel 304',185.75);
INSERT INTO machines (id, name, hourly_rate, ideal_units_per_hour, setup_cost_per_run) VALUES
    (1,'CNC Milling M1',480.00,145.0,1500.00),
    (2,'Injection Moulder M2',350.00,360.0,2200.00),
    (3,'Lathe M3',260.00,112.0,900.00),
    (4,'Press Brake M4',300.00,180.0,1100.00),
    (5,'Laser Cutter M5',620.00,225.0,2600.00);
INSERT INTO production_runs
    (run_date,machine_id,material_id,quantity_produced,quantity_rejected,machine_hours,material_kg_used) VALUES
    ('2026-01-08',1,1, 900, 40, 8.5, 640),('2026-01-15',2,4,2400, 95, 9.0, 520),
    ('2026-01-22',3,2, 600, 30, 7.5, 300),('2026-02-05',4,1,1500, 55,11.0, 980),
    ('2026-02-12',5,5, 800, 18, 4.5, 420),('2026-02-19',1,1,1100, 35, 9.8, 760),
    ('2026-02-26',2,4,3000,110,10.5, 640),('2026-03-04',3,2, 700, 22, 8.2, 340),
    ('2026-03-11',4,1,1800, 48,12.5,1120),('2026-03-18',5,5, 950, 15, 5.1, 480),
    ('2026-03-25',1,1,1300, 28,10.4, 850),('2026-04-02',2,4,3600, 88,11.8, 720),
    ('2026-04-09',3,2, 850, 19, 9.0, 400),('2026-04-16',4,1,2100, 40,13.8,1250),
    ('2026-04-23',5,5,1150, 12, 5.9, 560),('2026-04-30',1,1,1600, 25,11.9, 980),
    ('2026-05-07',2,4,4200, 72,12.9, 810),('2026-05-14',3,2,1000, 14, 9.9, 460),
    ('2026-05-21',4,1,2500, 33,15.2,1400),('2026-05-28',5,5,1400,  9, 6.8, 640),
    ('2026-06-04',1,1,1900, 20,13.1,1100),('2026-06-11',2,4,4800, 60,14.0, 900),
    ('2026-06-18',3,2,1150, 11,10.6, 500),('2026-06-25',4,1,2900, 27,16.5,1520),
    ('2026-06-30',5,5,1650,  7, 7.6, 720),('2026-07-06',1,1,2200, 16,14.2,1220),
    ('2026-07-13',2,4,5400, 48,15.1, 980),('2026-07-20',3,2,1300,  9,11.2, 540),
    ('2026-07-27',4,1,3300, 22,17.8,1650),('2026-08-03',5,5,1900,  5, 8.4, 800);
)SQL";
    return execute(seed, error);
}
