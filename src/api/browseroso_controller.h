/**
 * @file browseroso_controller.h
 * @brief Browseroso controller - Web interface for browsing tables
 */

#pragma once

#include <httplib.h>
#include <string>

namespace Tootega
{
namespace Api
{

/**
 * @class BrowserosoController
 * @brief Handles browseroso endpoints and UI
 */
class BrowserosoController
{
  public:
    /**
     * @brief Register all browseroso routes
     * @param server HTTP server instance
     */
    static void registerRoutes(httplib::Server &server);

  private:
    // API Endpoints
    static void getBrowserosoUI(const httplib::Request &req, httplib::Response &res);
    static void connectDatabase(const httplib::Request &req, httplib::Response &res);
    static void disconnectDatabase(const httplib::Request &req, httplib::Response &res);
    static void getConnectionStatus(const httplib::Request &req, httplib::Response &res);
    static void getTables(const httplib::Request &req, httplib::Response &res);
    static void getTableColumns(const httplib::Request &req, httplib::Response &res);
    static void getTableData(const httplib::Request &req, httplib::Response &res);

    // HTML Generation
    static std::string generateBrowserosoHTML();
};

} // namespace Api
} // namespace Tootega
