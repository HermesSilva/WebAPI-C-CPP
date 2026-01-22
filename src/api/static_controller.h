/**
 * @file static_controller.h
 * @brief Static file serving controller
 */

#pragma once

#include <httplib.h>
#include <string>

namespace Tootega
{
namespace Api
{

/**
 * @brief Controller for serving static files (CSS, JS, HTML)
 */
class StaticController
{
  public:
    /**
     * @brief Register static file routes
     */
    static void registerRoutes(httplib::Server &server);

    /**
     * @brief Set the base path for static files
     */
    static void setBasePath(const std::string &path);

    /**
     * @brief Get the current base path
     */
    static std::string getBasePath();

  private:
    static std::string s_basePath;

    static void serveStaticFile(const httplib::Request &req, httplib::Response &res);
    static void serveHomePage(const httplib::Request &req, httplib::Response &res);
    static void serveLoginPage(const httplib::Request &req, httplib::Response &res);
    static void serveBrowserPage(const httplib::Request &req, httplib::Response &res);

    static std::string readFile(const std::string &path);
    static std::string getMimeType(const std::string &path);
    static bool fileExists(const std::string &path);
};

} // namespace Api
} // namespace Tootega
