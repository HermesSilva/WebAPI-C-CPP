/**
 * @file server.cpp
 * @brief HTTP Server implementation
 */

#include "server.h"
#include "api/auth_controller.h"
#include "api/browser_controller.h"
#include "api/docs_controller.h"
#include "api/static_controller.h"
#include "api/version_controller.h"

#include <httplib.h>
#include <iostream>

namespace Tootega
{
namespace Core
{

Server::Server(const std::string &host, int port)
    : m_host(host), m_port(port), m_running(false), m_server(std::make_unique<httplib::Server>())
{
    setupCORS();
    setupRoutes();
}

Server::~Server()
{
    stop();
}

void Server::setupCORS()
{
    // Pre-flight OPTIONS handler for CORS
    m_server->Options(R"(.*)", [](const httplib::Request & /*req*/, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 204;
    });

    // Pre-routing handler: check JWT authentication for protected routes
    m_server->set_pre_routing_handler([](const httplib::Request &req, httplib::Response &res) {
        // Skip OPTIONS requests (CORS preflight)
        if (req.method == "OPTIONS")
        {
            return httplib::Server::HandlerResponse::Unhandled;
        }

        // Check if route requires authentication
        if (Api::AuthController::requiresAuth(req.path))
        {
            if (!Api::AuthController::isAuthenticated(req))
            {
                // Check if this is an API request (expects JSON) or browser request
                bool isApiRequest = req.has_header("Accept") &&
                                    req.get_header_value("Accept").find("application/json") != std::string::npos;

                // Also check X-Requested-With for AJAX requests
                if (!isApiRequest && req.has_header("X-Requested-With"))
                {
                    isApiRequest = true;
                }

                // Check if Authorization header is present (API client)
                if (!isApiRequest && req.has_header("Authorization"))
                {
                    isApiRequest = true;
                }

                if (isApiRequest)
                {
                    // Return JSON error for API requests
                    res.status = 401;
                    res.set_content(R"({"error": "Unauthorized", "message": "Valid JWT token required"})",
                                    "application/json");
                }
                else
                {
                    // Redirect to login page for browser requests
                    res.status = 302;
                    res.set_header("Location", "/login?redirect=" + req.path);
                    res.set_content("", "text/html");
                }

                return httplib::Server::HandlerResponse::Handled;
            }
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Add CORS headers to all responses
    m_server->set_post_routing_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });
}

void Server::setupRoutes()
{
    // Health check endpoint
    m_server->Get("/health", [](const httplib::Request & /*req*/, httplib::Response &res) {
        res.set_content(R"({"status": "healthy"})", "application/json");
    });

    // Register controllers - ORDER MATTERS!
    // Static controller first for HTML pages and static files
    Api::StaticController::registerRoutes(*m_server);

    // Auth controller for authentication endpoints
    Api::AuthController::registerRoutes(*m_server);

    // API controllers
    Api::VersionController::registerRoutes(*m_server);
    Api::DocsController::registerRoutes(*m_server);
    Api::BrowserController::registerRoutes(*m_server);

    // 404 handler
    m_server->set_error_handler([](const httplib::Request &req, httplib::Response &res) {
        std::string json = R"({
    "error": "Not Found",
    "message": "The requested resource was not found",
    "path": ")" + req.path +
                           R"(",
    "status": 404
})";
        res.set_content(json, "application/json");
        res.status = 404;
    });

    // Exception handler
    m_server->set_exception_handler(
        [](const httplib::Request & /*req*/, httplib::Response &res, std::exception_ptr ep) {
            std::string message = "Unknown error";
            try
            {
                if (ep)
                {
                    std::rethrow_exception(ep);
                }
            }
            catch (const std::exception &e)
            {
                message = e.what();
            }

            std::string json = R"({
    "error": "Internal Server Error",
    "message": ")" + message + R"(",
    "status": 500
})";
            res.set_content(json, "application/json");
            res.status = 500;
        });

    // Logging
    m_server->set_logger([](const httplib::Request &req, const httplib::Response &res) {
        std::cout << "[" << req.method << "] " << req.path << " -> " << res.status << std::endl;
    });
}

bool Server::start()
{
    if (m_running)
    {
        return false;
    }

    m_running = true;

    // Listen and serve (blocking call)
    bool result = m_server->listen(m_host, m_port);

    m_running = false;
    return result;
}

void Server::stop()
{
    if (m_running && m_server)
    {
        m_server->stop();
        m_running = false;
    }
}

bool Server::isRunning() const
{
    return m_running;
}

} // namespace Core
} // namespace Tootega
