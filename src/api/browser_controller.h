/**
 * @file browser_controller.h
 * @brief Database browser controller - Web interface for browsing tables
 */

#pragma once

#include <httplib.h>
#include <string>

namespace Tootega
{
namespace Api
{

/**
 * @class BrowserController
 * @brief Handles database browser endpoints and UI
 */
class BrowserController
{
  public:
    /**
     * @brief Register all browser routes
     * @param server HTTP server instance
     */
    static void registerRoutes(httplib::Server &server);

  private:
    // API Endpoints
    static void getBrowserUI(const httplib::Request &req, httplib::Response &res);
    static void connectDatabase(const httplib::Request &req, httplib::Response &res);
    static void disconnectDatabase(const httplib::Request &req, httplib::Response &res);
    static void getConnectionStatus(const httplib::Request &req, httplib::Response &res);
    static void getTables(const httplib::Request &req, httplib::Response &res);
    static void getTableColumns(const httplib::Request &req, httplib::Response &res);
    static void getTableData(const httplib::Request &req, httplib::Response &res);

    // HTML Generation
    static std::string generateBrowserHTML();
};

} // namespace Api
} // namespace Tootega
