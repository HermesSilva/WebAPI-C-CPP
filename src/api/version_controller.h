/**
 * @file version_controller.h
 * @brief Version API endpoint controller
 */

#pragma once

#include <httplib.h>

namespace Tootega
{
namespace Api
{

/**
 * @class VersionController
 * @brief Handles /api/version endpoints
 */
class VersionController
{
  public:
    /**
     * @brief Register all version-related routes
     * @param server HTTP server instance
     */
    static void registerRoutes(httplib::Server &server);

  private:
    /**
     * @brief GET /api/version - Returns system version information
     */
    static void getVersion(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief GET /api/version/detailed - Returns detailed version information
     */
    static void getVersionDetailed(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief GET /api/health - Returns health status
     */
    static void getHealth(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief Helper to escape JSON strings
     */
    static std::string escapeJson(const std::string &str);
};

} // namespace Api
} // namespace Tootega
