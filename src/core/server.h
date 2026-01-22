/**
 * @file server.h
 * @brief HTTP Server class declaration
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>

// Forward declaration
namespace httplib
{
class Server;
}

namespace Tootega
{
namespace Core
{

/**
 * @class Server
 * @brief HTTP Server wrapper for the WebAPI
 */
class Server
{
  public:
    /**
     * @brief Construct a new Server object
     * @param host Bind address
     * @param port Port number
     */
    Server(const std::string &host, int port);

    /**
     * @brief Destroy the Server object
     */
    ~Server();

    // Disable copy
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    /**
     * @brief Start the server (blocking)
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool isRunning() const;

    /**
     * @brief Get the host address
     * @return const std::string&
     */
    const std::string &getHost() const
    {
        return m_host;
    }

    /**
     * @brief Get the port number
     * @return int
     */
    int getPort() const
    {
        return m_port;
    }

  private:
    /**
     * @brief Setup all API routes
     */
    void setupRoutes();

    /**
     * @brief Setup CORS headers
     */
    void setupCORS();

    std::string m_host;
    int m_port;
    std::atomic<bool> m_running;
    std::unique_ptr<httplib::Server> m_server;
};

} // namespace Core
} // namespace Tootega
