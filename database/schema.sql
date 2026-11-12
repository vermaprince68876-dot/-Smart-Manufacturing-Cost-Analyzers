-- Smart Manufacturing Cost Analyzer - schema
-- Reference copy of the DDL the program creates in DatabaseManager::ensureSchema().
-- Run with:  sqlite3 factory.db < database/schema.sql

PRAGMA foreign_keys = ON;

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
