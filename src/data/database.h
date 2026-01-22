/**
 * @file database.h
 * @brief Database connection and query management
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Tootega
{
namespace Data
{

/**
 * @brief Represents a database column metadata
 */
struct ColumnInfo
{
    std::string name;
    std::string type;
    bool nullable;
    bool isPrimaryKey;
};

/**
 * @brief Represents a database table metadata
 */
struct TableInfo
{
    std::string schema;
    std::string name;
    std::vector<ColumnInfo> columns;
};

/**
 * @brief Represents a row of data
 */
using DataRow = std::vector<std::pair<std::string, std::string>>;

/**
 * @brief Represents query results
 */
struct QueryResult
{
    std::vector<std::string> columns;
    std::vector<DataRow> rows;
    int totalRows;
    std::string error;
    bool success;
};

/**
 * @class Database
 * @brief Manages database connections and queries using ODBC
 */
class Database
{
  public:
    /**
     * @brief Get the singleton instance
     */
    static Database &getInstance();

    /**
     * @brief Initialize database connection
     * @param connectionString ODBC connection string
     * @return true if connection successful
     */
    bool connect(const std::string &connectionString);

    /**
     * @brief Check if connected
     */
    bool isConnected() const;

    /**
     * @brief Disconnect from database
     */
    void disconnect();

    /**
     * @brief Get list of all databases on the server
     */
    std::vector<std::string> getDatabases();

    /**
     * @brief Change the current database
     * @param databaseName Name of the database to switch to
     * @return true if successful
     */
    bool useDatabase(const std::string &databaseName);

    /**
     * @brief Get list of all tables in database
     */
    std::vector<TableInfo> getTables();

    /**
     * @brief Get columns for a specific table
     */
    std::vector<ColumnInfo> getColumns(const std::string &schema, const std::string &tableName);

    /**
     * @brief Execute a SELECT query with optional filters
     * @param tableName Table to query
     * @param filterColumn Column to filter (empty for no filter)
     * @param filterValue Value to filter
     * @param page Page number (1-based)
     * @param pageSize Number of rows per page
     */
    QueryResult selectData(const std::string &schema, const std::string &tableName,
                           const std::string &filterColumn = "", const std::string &filterValue = "", int page = 1,
                           int pageSize = 50);

    /**
     * @brief Get the connection string (masked)
     */
    std::string getConnectionInfo() const;

    // Delete copy constructor and assignment
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

  private:
    Database();
    ~Database();

    class Impl;
    std::unique_ptr<Impl> m_impl;
    mutable std::mutex m_mutex;
};

} // namespace Data
} // namespace Tootega
