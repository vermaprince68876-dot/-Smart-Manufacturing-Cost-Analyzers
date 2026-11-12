// DatabaseManager - the only class that knows SQLite exists.
// It owns one connection, closes it in its destructor, and hands out
// prepared statements to the services layer.

#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>

// Owns one SQLite connection and closes it in the destructor.
class DatabaseManager {
public:
    DatabaseManager() = default;
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&)            = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool open(const std::string& path, std::string& error);
    void close();

    bool execute(const std::string& sql, std::string& error);
    sqlite3_stmt* prepare(const std::string& sql, std::string& error);
    long countRows(const std::string& table, std::string& error);
    std::string lastError() const;

    bool ensureSchema(std::string& error);
    bool seedSampleData(std::string& error);

private:
    sqlite3* db_ = nullptr;
};

#endif // DATABASE_MANAGER_H
