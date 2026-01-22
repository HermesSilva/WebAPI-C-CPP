/**
 * @file connection_manager.cpp
 * @brief Session-based database connection manager implementation
 */

#include "connection_manager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#ifdef PLATFORM_WINDOWS
#undef UNICODE
#undef _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

namespace Tootega
{
namespace Data
{

/**
 * @brief Convert Windows-1252/Latin-1 string to UTF-8
 */
static std::string latin1ToUtf8(const std::string &latin1)
{
#ifdef PLATFORM_WINDOWS
    if (latin1.empty())
        return latin1;

    int wideLen = MultiByteToWideChar(1252, 0, latin1.c_str(), -1, NULL, 0);
    if (wideLen == 0)
        return latin1;

    std::wstring wide(wideLen, 0);
    MultiByteToWideChar(1252, 0, latin1.c_str(), -1, &wide[0], wideLen);

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Len == 0)
        return latin1;

    std::string utf8(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8Len, NULL, NULL);

    if (!utf8.empty() && utf8.back() == '\0')
        utf8.pop_back();

    return utf8;
#else
    return latin1;
#endif
}

/**
 * @brief ODBC handle wrapper for RAII
 */
template <typename HandleType> class OdbcHandle
{
  public:
    OdbcHandle() : m_handle(SQL_NULL_HANDLE)
    {
    }
    ~OdbcHandle()
    {
        free();
    }

    HandleType get() const
    {
        return m_handle;
    }
    HandleType *ptr()
    {
        return &m_handle;
    }

    void free()
    {
        if (m_handle != SQL_NULL_HANDLE)
        {
            if constexpr (std::is_same_v<HandleType, SQLHENV>)
            {
                SQLFreeHandle(SQL_HANDLE_ENV, m_handle);
            }
            else if constexpr (std::is_same_v<HandleType, SQLHDBC>)
            {
                SQLFreeHandle(SQL_HANDLE_DBC, m_handle);
            }
            else if constexpr (std::is_same_v<HandleType, SQLHSTMT>)
            {
                SQLFreeHandle(SQL_HANDLE_STMT, m_handle);
            }
            m_handle = SQL_NULL_HANDLE;
        }
    }

    operator bool() const
    {
        return m_handle != SQL_NULL_HANDLE;
    }

  private:
    HandleType m_handle;
};

/**
 * @brief DatabaseConnection implementation
 */
class DatabaseConnection::Impl
{
  public:
    Impl() : m_connected(false)
    {
    }

    ~Impl()
    {
        disconnect();
    }

    bool connect(const std::string &connectionString)
    {
        disconnect();

        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, m_env.ptr());
        if (!SQL_SUCCEEDED(ret))
        {
            m_lastError = "Failed to allocate environment handle";
            return false;
        }

        ret = SQLSetEnvAttr(m_env.get(), SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (!SQL_SUCCEEDED(ret))
        {
            m_lastError = "Failed to set ODBC version";
            return false;
        }

        ret = SQLAllocHandle(SQL_HANDLE_DBC, m_env.get(), m_dbc.ptr());
        if (!SQL_SUCCEEDED(ret))
        {
            m_lastError = "Failed to allocate connection handle";
            return false;
        }

        SQLSetConnectAttr(m_dbc.get(), SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

        std::string odbcConnStr = convertConnectionString(connectionString);

        SQLCHAR outConnStr[1024];
        SQLSMALLINT outConnStrLen;

        ret = SQLDriverConnect(m_dbc.get(), NULL, (SQLCHAR *)odbcConnStr.c_str(), SQL_NTS, outConnStr,
                               sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_NOPROMPT);

        if (!SQL_SUCCEEDED(ret))
        {
            m_lastError = getOdbcError(SQL_HANDLE_DBC, m_dbc.get());
            return false;
        }

        m_connectionString = connectionString;
        m_connected = true;
        return true;
    }

    void disconnect()
    {
        if (m_connected)
        {
            SQLDisconnect(m_dbc.get());
            m_connected = false;
        }
        m_dbc.free();
        m_env.free();
    }

    bool isConnected() const
    {
        return m_connected;
    }

    std::vector<std::string> getDatabases()
    {
        std::vector<std::string> databases;

        if (!m_connected)
            return databases;

        OdbcHandle<SQLHSTMT> stmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
            return databases;

        const char *sql = "SELECT name FROM sys.databases WHERE state_desc = 'ONLINE' ORDER BY name";
        ret = SQLExecDirect(stmt.get(), (SQLCHAR *)sql, SQL_NTS);
        if (!SQL_SUCCEEDED(ret))
            return databases;

        SQLCHAR dbName[256];
        SQLLEN dbNameLen;

        while (SQLFetch(stmt.get()) == SQL_SUCCESS)
        {
            SQLGetData(stmt.get(), 1, SQL_C_CHAR, dbName, sizeof(dbName), &dbNameLen);
            databases.push_back(std::string((char *)dbName));
        }

        return databases;
    }

    bool useDatabase(const std::string &databaseName)
    {
        if (!m_connected)
            return false;

        if (!isValidIdentifier(databaseName))
            return false;

        OdbcHandle<SQLHSTMT> stmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
            return false;

        std::string sql = "USE [" + databaseName + "]";
        ret = SQLExecDirect(stmt.get(), (SQLCHAR *)sql.c_str(), SQL_NTS);

        if (SQL_SUCCEEDED(ret))
        {
            auto pos = m_connectionString.find("Initial Catalog=");
            if (pos != std::string::npos)
            {
                auto end = m_connectionString.find(';', pos);
                m_connectionString = m_connectionString.substr(0, pos) + "Initial Catalog=" + databaseName +
                                     m_connectionString.substr(end);
            }
            return true;
        }

        return false;
    }

    std::vector<TableInfo> getTables()
    {
        std::vector<TableInfo> tables;

        if (!m_connected)
            return tables;

        OdbcHandle<SQLHSTMT> stmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
            return tables;

        ret = SQLTables(stmt.get(), NULL, 0, NULL, 0, NULL, 0, (SQLCHAR *)"TABLE", SQL_NTS);
        if (!SQL_SUCCEEDED(ret))
            return tables;

        SQLCHAR schemaName[256], tableName[256];
        SQLLEN schemaLen, tableLen;

        while (SQLFetch(stmt.get()) == SQL_SUCCESS)
        {
            SQLGetData(stmt.get(), 2, SQL_C_CHAR, schemaName, sizeof(schemaName), &schemaLen);
            SQLGetData(stmt.get(), 3, SQL_C_CHAR, tableName, sizeof(tableName), &tableLen);

            TableInfo info;
            info.schema = (schemaLen > 0) ? std::string((char *)schemaName) : "dbo";
            info.name = std::string((char *)tableName);

            if (info.schema != "sys" && info.schema != "INFORMATION_SCHEMA")
            {
                tables.push_back(info);
            }
        }

        return tables;
    }

    std::vector<ColumnInfo> getColumns(const std::string &schema, const std::string &tableName)
    {
        std::vector<ColumnInfo> columns;

        if (!m_connected)
            return columns;

        OdbcHandle<SQLHSTMT> stmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
            return columns;

        ret = SQLColumns(stmt.get(), NULL, 0, (SQLCHAR *)schema.c_str(), SQL_NTS, (SQLCHAR *)tableName.c_str(), SQL_NTS,
                         NULL, 0);

        if (!SQL_SUCCEEDED(ret))
            return columns;

        SQLCHAR colName[256], typeName[256];
        SQLSMALLINT nullable;
        SQLLEN colNameLen, typeNameLen, nullableLen;

        while (SQLFetch(stmt.get()) == SQL_SUCCESS)
        {
            SQLGetData(stmt.get(), 4, SQL_C_CHAR, colName, sizeof(colName), &colNameLen);
            SQLGetData(stmt.get(), 6, SQL_C_CHAR, typeName, sizeof(typeName), &typeNameLen);
            SQLGetData(stmt.get(), 11, SQL_C_SSHORT, &nullable, 0, &nullableLen);

            ColumnInfo col;
            col.name = std::string((char *)colName);
            col.type = std::string((char *)typeName);
            col.nullable = (nullable == SQL_NULLABLE);
            col.isPrimaryKey = false;

            columns.push_back(col);
        }

        // Get primary keys
        stmt.free();
        ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (SQL_SUCCEEDED(ret))
        {
            ret = SQLPrimaryKeys(stmt.get(), NULL, 0, (SQLCHAR *)schema.c_str(), SQL_NTS, (SQLCHAR *)tableName.c_str(),
                                 SQL_NTS);

            if (SQL_SUCCEEDED(ret))
            {
                SQLCHAR pkColName[256];
                SQLLEN pkColNameLen;

                while (SQLFetch(stmt.get()) == SQL_SUCCESS)
                {
                    SQLGetData(stmt.get(), 4, SQL_C_CHAR, pkColName, sizeof(pkColName), &pkColNameLen);
                    std::string pkName((char *)pkColName);

                    for (auto &col : columns)
                    {
                        if (col.name == pkName)
                        {
                            col.isPrimaryKey = true;
                            break;
                        }
                    }
                }
            }
        }

        return columns;
    }

    QueryResult selectData(const std::string &schema, const std::string &tableName, const std::string &filterColumn,
                           const std::string &filterValue, int page, int pageSize)
    {
        QueryResult result;
        result.success = false;
        result.totalRows = 0;

        if (!m_connected)
        {
            result.error = "Not connected to database";
            return result;
        }

        if (!isValidIdentifier(tableName) || (!schema.empty() && !isValidIdentifier(schema)))
        {
            result.error = "Invalid table or schema name";
            return result;
        }

        if (!filterColumn.empty() && !isValidIdentifier(filterColumn))
        {
            result.error = "Invalid column name";
            return result;
        }

        std::string fullTableName = schema.empty() ? "[" + tableName + "]" : "[" + schema + "].[" + tableName + "]";

        // First, get total count
        std::string countSql = "SELECT COUNT(*) FROM " + fullTableName;
        if (!filterColumn.empty() && !filterValue.empty())
        {
            countSql += " WHERE [" + filterColumn + "] LIKE ?";
        }

        OdbcHandle<SQLHSTMT> stmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
        {
            result.error = "Failed to allocate statement handle";
            return result;
        }

        ret = SQLPrepare(stmt.get(), (SQLCHAR *)countSql.c_str(), SQL_NTS);
        if (!SQL_SUCCEEDED(ret))
        {
            result.error = getOdbcError(SQL_HANDLE_STMT, stmt.get());
            return result;
        }

        std::string likeValue = "%" + filterValue + "%";
        if (!filterColumn.empty() && !filterValue.empty())
        {
            SQLBindParameter(stmt.get(), 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, likeValue.size(), 0,
                             (SQLCHAR *)likeValue.c_str(), likeValue.size(), NULL);
        }

        ret = SQLExecute(stmt.get());
        if (SQL_SUCCEEDED(ret) && SQLFetch(stmt.get()) == SQL_SUCCESS)
        {
            SQLINTEGER count;
            SQLGetData(stmt.get(), 1, SQL_C_SLONG, &count, 0, NULL);
            result.totalRows = count;
        }

        // Now get the actual data with pagination
        int offset = (page - 1) * pageSize;

        std::string dataSql = "SELECT * FROM " + fullTableName;
        if (!filterColumn.empty() && !filterValue.empty())
        {
            dataSql += " WHERE [" + filterColumn + "] LIKE ?";
        }
        dataSql += " ORDER BY (SELECT NULL) OFFSET " + std::to_string(offset) + " ROWS FETCH NEXT " +
                   std::to_string(pageSize) + " ROWS ONLY";

        stmt.free();
        ret = SQLAllocHandle(SQL_HANDLE_STMT, m_dbc.get(), stmt.ptr());
        if (!SQL_SUCCEEDED(ret))
        {
            result.error = "Failed to allocate statement handle";
            return result;
        }

        ret = SQLPrepare(stmt.get(), (SQLCHAR *)dataSql.c_str(), SQL_NTS);
        if (!SQL_SUCCEEDED(ret))
        {
            result.error = getOdbcError(SQL_HANDLE_STMT, stmt.get());
            return result;
        }

        if (!filterColumn.empty() && !filterValue.empty())
        {
            SQLBindParameter(stmt.get(), 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, likeValue.size(), 0,
                             (SQLCHAR *)likeValue.c_str(), likeValue.size(), NULL);
        }

        ret = SQLExecute(stmt.get());
        if (!SQL_SUCCEEDED(ret))
        {
            result.error = getOdbcError(SQL_HANDLE_STMT, stmt.get());
            return result;
        }

        // Get column names
        SQLSMALLINT numCols;
        SQLNumResultCols(stmt.get(), &numCols);

        for (SQLSMALLINT i = 1; i <= numCols; i++)
        {
            SQLCHAR colName[256];
            SQLSMALLINT nameLen;
            SQLDescribeCol(stmt.get(), i, colName, sizeof(colName), &nameLen, NULL, NULL, NULL, NULL);
            result.columns.push_back(std::string((char *)colName));
        }

        // Fetch data
        while (SQLFetch(stmt.get()) == SQL_SUCCESS)
        {
            DataRow row;
            for (SQLSMALLINT i = 1; i <= numCols; i++)
            {
                SQLCHAR value[4096];
                SQLLEN valueLen;

                ret = SQLGetData(stmt.get(), i, SQL_C_CHAR, value, sizeof(value), &valueLen);

                std::string val;
                if (valueLen == SQL_NULL_DATA)
                {
                    val = "NULL";
                }
                else if (SQL_SUCCEEDED(ret))
                {
                    val = latin1ToUtf8(std::string((char *)value));
                }

                row.push_back({result.columns[i - 1], val});
            }
            result.rows.push_back(row);
        }

        result.success = true;
        return result;
    }

    std::string getConnectionInfo() const
    {
        if (!m_connected)
            return "Not connected";

        std::string info = "Connected";

        auto pos = m_connectionString.find("Initial Catalog=");
        if (pos != std::string::npos)
        {
            auto end = m_connectionString.find(';', pos);
            info += " to " + m_connectionString.substr(pos + 16, end - pos - 16);
        }

        return info;
    }

  private:
    std::string convertConnectionString(const std::string &adoConnStr)
    {
        std::ostringstream odbc;
        odbc << "DRIVER={ODBC Driver 17 for SQL Server};";

        std::istringstream ss(adoConnStr);
        std::string pair;

        while (std::getline(ss, pair, ';'))
        {
            if (pair.empty())
                continue;

            auto eqPos = pair.find('=');
            if (eqPos == std::string::npos)
                continue;

            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);

            if (key == "Data Source" || key == "Server")
            {
                odbc << "SERVER=" << value << ";";
            }
            else if (key == "Initial Catalog" || key == "Database")
            {
                odbc << "DATABASE=" << value << ";";
            }
            else if (key == "Integrated Security" && (value == "True" || value == "SSPI"))
            {
                odbc << "Trusted_Connection=yes;";
            }
            else if (key == "User ID" || key == "User Id")
            {
                odbc << "UID=" << value << ";";
            }
            else if (key == "Password")
            {
                odbc << "PWD=" << value << ";";
            }
            else if (key == "Encrypt" && value == "False")
            {
                odbc << "Encrypt=no;";
            }
            else if (key == "TrustServerCertificate" && value == "True")
            {
                odbc << "TrustServerCertificate=yes;";
            }
        }

        return odbc.str();
    }

    std::string getOdbcError(SQLSMALLINT handleType, SQLHANDLE handle)
    {
        SQLCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT messageLen;

        SQLRETURN ret =
            SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError, message, sizeof(message), &messageLen);

        if (SQL_SUCCEEDED(ret))
        {
            return std::string((char *)sqlState) + ": " + std::string((char *)message);
        }
        return "Unknown ODBC error";
    }

    bool isValidIdentifier(const std::string &name)
    {
        if (name.empty())
            return false;
        for (char c : name)
        {
            if (!std::isalnum(c) && c != '_')
                return false;
        }
        return true;
    }

    OdbcHandle<SQLHENV> m_env;
    OdbcHandle<SQLHDBC> m_dbc;
    std::string m_connectionString;
    std::string m_lastError;
    bool m_connected;
};

// DatabaseConnection public methods
DatabaseConnection::DatabaseConnection() : m_impl(std::make_unique<Impl>())
{
}

DatabaseConnection::~DatabaseConnection() = default;

bool DatabaseConnection::connect(const std::string &connectionString)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->connect(connectionString);
}

void DatabaseConnection::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_impl->disconnect();
}

bool DatabaseConnection::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->isConnected();
}

std::vector<std::string> DatabaseConnection::getDatabases()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->getDatabases();
}

bool DatabaseConnection::useDatabase(const std::string &databaseName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->useDatabase(databaseName);
}

std::vector<TableInfo> DatabaseConnection::getTables()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->getTables();
}

std::vector<ColumnInfo> DatabaseConnection::getColumns(const std::string &schema, const std::string &tableName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->getColumns(schema, tableName);
}

QueryResult DatabaseConnection::selectData(const std::string &schema, const std::string &tableName,
                                           const std::string &filterColumn, const std::string &filterValue, int page,
                                           int pageSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->selectData(schema, tableName, filterColumn, filterValue, page, pageSize);
}

std::string DatabaseConnection::getConnectionInfo() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_impl->getConnectionInfo();
}

// ConnectionManager implementation
ConnectionManager &ConnectionManager::getInstance()
{
    static ConnectionManager instance;
    return instance;
}

DatabaseConnection &ConnectionManager::getConnection(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end())
    {
        SessionInfo info;
        info.connection = std::make_unique<DatabaseConnection>();
        info.lastAccess = std::chrono::steady_clock::now();
        auto [newIt, _] = m_sessions.emplace(sessionId, std::move(info));
        return *newIt->second.connection;
    }

    it->second.lastAccess = std::chrono::steady_clock::now();
    return *it->second.connection;
}

bool ConnectionManager::hasConnection(const std::string &sessionId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    return it != m_sessions.end() && it->second.connection->isConnected();
}

void ConnectionManager::removeConnection(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(sessionId);
}

void ConnectionManager::cleanupExpiredSessions(int maxIdleSeconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_sessions.begin(); it != m_sessions.end();)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastAccess).count();
        if (elapsed > maxIdleSeconds)
        {
            it = m_sessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace Data
} // namespace Tootega
