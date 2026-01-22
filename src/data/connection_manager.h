/**
 * @file connection_manager.h
 * @brief Session-based database connection manager
 */

#pragma once

#include "database.h"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace Tootega
{
namespace Data
{

/**
 * @brief Individual database connection for a session
 */
class DatabaseConnection
{
  public:
    DatabaseConnection();
    ~DatabaseConnection();

    bool connect(const std::string &connectionString);
    void disconnect();
    bool isConnected() const;

    std::vector<std::string> getDatabases();
    bool useDatabase(const std::string &databaseName);
    std::vector<TableInfo> getTables();
    std::vector<ColumnInfo> getColumns(const std::string &schema, const std::string &tableName);
    QueryResult selectData(const std::string &schema, const std::string &tableName,
                           const std::string &filterColumn = "", const std::string &filterValue = "", int page = 1,
                           int pageSize = 50);
    std::string getConnectionInfo() const;

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    mutable std::mutex m_mutex;
};

/**
 * @brief Session info with connection and last access time
 */
struct SessionInfo
{
    std::unique_ptr<DatabaseConnection> connection;
    std::chrono::steady_clock::time_point lastAccess;
};

/**
 * @class ConnectionManager
 * @brief Manages database connections per session (stateless per user)
 */
class ConnectionManager
{
  public:
    /**
     * @brief Get the singleton instance
     */
    static ConnectionManager &getInstance();

    /**
     * @brief Get or create connection for a session
     * @param sessionId Unique session identifier (e.g., from JWT)
     * @return Reference to the database connection for this session
     */
    DatabaseConnection &getConnection(const std::string &sessionId);

    /**
     * @brief Check if session has an active connection
     */
    bool hasConnection(const std::string &sessionId) const;

    /**
     * @brief Remove connection for a session
     */
    void removeConnection(const std::string &sessionId);

    /**
     * @brief Clean up expired sessions (called periodically)
     * @param maxIdleSeconds Maximum idle time before session is removed
     */
    void cleanupExpiredSessions(int maxIdleSeconds = 3600);

    // Delete copy constructor and assignment
    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager &operator=(const ConnectionManager &) = delete;

  private:
    ConnectionManager() = default;
    ~ConnectionManager() = default;

    std::map<std::string, SessionInfo> m_sessions;
    mutable std::mutex m_mutex;
};

} // namespace Data
} // namespace Tootega
