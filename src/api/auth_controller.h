/**
 * @file auth_controller.h
 * @brief Authentication controller with JWT support
 */

#pragma once

#include <httplib.h>
#include <string>

namespace Tootega
{
namespace Api
{

/**
 * @brief Authentication controller
 */
class AuthController
{
  public:
    /**
     * @brief Register authentication routes
     */
    static void registerRoutes(httplib::Server &server);

    /**
     * @brief Middleware to verify JWT token (returns JSON error)
     * @return true if authenticated, false otherwise
     */
    static bool verifyAuth(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief Middleware to verify JWT token with redirect to login
     * @return true if authenticated, false and redirects to /login otherwise
     */
    static bool verifyAuthWithRedirect(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief Check if request has valid JWT (no response modification)
     * @return true if authenticated, false otherwise
     */
    static bool isAuthenticated(const httplib::Request &req);

    /**
     * @brief Check if path requires authentication
     */
    static bool requiresAuth(const std::string &path);

    /**
     * @brief Extract token from Authorization header
     */
    static std::string extractToken(const httplib::Request &req);

    /**
     * @brief Get the login page HTML
     */
    static std::string getLoginPageHTML();

  private:
    static void handleLogin(const httplib::Request &req, httplib::Response &res);
    static void handleLogout(const httplib::Request &req, httplib::Response &res);
    static void handleVerify(const httplib::Request &req, httplib::Response &res);
    static void handleLoginPage(const httplib::Request &req, httplib::Response &res);
    static void handleRefreshToken(const httplib::Request &req, httplib::Response &res);
};

} // namespace Api
} // namespace Tootega
