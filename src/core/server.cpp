/**
 * @file server.cpp
 * @brief HTTP Server implementation
 */

#include "server.h"
#include "api/auth_controller.h"
#include "api/browser_controller.h"
#include "api/docs_controller.h"
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
    // Root endpoint - Home page
    m_server->Get("/", [](const httplib::Request & /*req*/, httplib::Response &res) {
        std::string html = R"(<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tootega WebAPI</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: linear-gradient(135deg, #0f0f23 0%, #1a1a2e 100%);
            color: #e0e0e0;
            min-height: 100vh;
        }
        .header {
            background: linear-gradient(135deg, #1a2a4a 0%, #1e3a5f 100%);
            padding: 1rem 2rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px solid #333;
        }
        .header h1 { font-size: 1.5rem; color: #fff; }
        .header h1 span { color: #4da6ff; }
        .nav-links { display: flex; gap: 1rem; }
        .nav-links a {
            color: #aaa;
            text-decoration: none;
            padding: 0.5rem 1rem;
            border-radius: 6px;
            transition: all 0.2s;
        }
        .nav-links a:hover { background: rgba(255,255,255,0.1); color: #fff; }
        .nav-links a.active { background: rgba(45,125,210,0.3); color: #6bb8ff; }
        .hero {
            text-align: center;
            padding: 4rem 2rem;
            background: linear-gradient(180deg, rgba(45,125,210,0.1) 0%, transparent 100%);
        }
        .hero h2 {
            font-size: 2.5rem;
            margin-bottom: 1rem;
            color: #fff;
        }
        .hero h2 span { color: #4da6ff; }
        .hero p {
            font-size: 1.2rem;
            color: #888;
            max-width: 600px;
            margin: 0 auto 2rem;
        }
        .hero-buttons { display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap; }
        .btn {
            padding: 0.8rem 2rem;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1rem;
            text-decoration: none;
            transition: all 0.2s;
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
        }
        .btn-primary { background: #2d7dd2; color: white; }
        .btn-primary:hover { background: #1e5aa8; transform: translateY(-2px); }
        .btn-secondary { background: #333; color: #e0e0e0; border: 1px solid #444; }
        .btn-secondary:hover { background: #444; transform: translateY(-2px); }
        .features {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 2rem;
            padding: 3rem 2rem;
            max-width: 1200px;
            margin: 0 auto;
        }
        .feature-card {
            background: rgba(26, 26, 46, 0.8);
            border: 1px solid #333;
            border-radius: 12px;
            padding: 2rem;
            transition: all 0.3s;
        }
        .feature-card:hover {
            border-color: #2d7dd2;
            transform: translateY(-4px);
            box-shadow: 0 8px 32px rgba(45,125,210,0.2);
        }
        .feature-icon {
            font-size: 2.5rem;
            margin-bottom: 1rem;
        }
        .feature-card h3 {
            color: #fff;
            margin-bottom: 0.5rem;
            font-size: 1.25rem;
        }
        .feature-card p { color: #888; line-height: 1.6; }
        .feature-card a {
            display: inline-block;
            margin-top: 1rem;
            color: #4da6ff;
            text-decoration: none;
        }
        .feature-card a:hover { text-decoration: underline; }
        .footer {
            text-align: center;
            padding: 2rem;
            border-top: 1px solid #333;
            color: #666;
        }
        .footer a { color: #4da6ff; text-decoration: none; }
        .footer a:hover { text-decoration: underline; }
        .status-badge {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            background: rgba(34, 197, 94, 0.2);
            color: #22c55e;
            padding: 0.25rem 0.75rem;
            border-radius: 20px;
            font-size: 0.875rem;
            margin-left: 1rem;
        }
        .status-dot {
            width: 8px;
            height: 8px;
            background: #22c55e;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
    </style>
</head>
<body>
    <header class="header">
        <h1>Tootega <span>WebAPI</span> <span class="status-badge"><span class="status-dot"></span>Online</span></h1>
        <nav class="nav-links">
            <a href="/" class="active">Home</a>
            <a href="/api/docs">API Docs</a>
            <a href="/browseroso">Database Browser</a>
            <a href="/login">Login</a>
        </nav>
    </header>

    <section class="hero">
        <h2>Bem-vindo ao <span>Tootega WebAPI</span></h2>
        <p>Sistema de gerenciamento e API para acesso a dados. Navegue pelo banco de dados, consulte a documentacao ou faca login para acessar recursos protegidos.</p>
        <div class="hero-buttons">
            <a href="/browseroso" class="btn btn-primary">&#128202; Acessar Browser</a>
            <a href="/api/docs" class="btn btn-secondary">&#128214; Documentacao</a>
        </div>
    </section>

    <section class="features">
        <div class="feature-card">
            <div class="feature-icon">&#128202;</div>
            <h3>Database Browser</h3>
            <p>Navegue pelas tabelas do banco de dados, visualize dados, filtre e exporte informacoes de forma intuitiva.</p>
            <a href="/browseroso">Acessar Browser &rarr;</a>
        </div>
        <div class="feature-card">
            <div class="feature-icon">&#128214;</div>
            <h3>API Documentation</h3>
            <p>Documentacao completa da API REST com exemplos de uso e descricao de todos os endpoints disponiveis.</p>
            <a href="/api/docs">Ver Documentacao &rarr;</a>
        </div>
        <div class="feature-card">
            <div class="feature-icon">&#128274;</div>
            <h3>Autenticacao JWT</h3>
            <p>Sistema seguro de autenticacao com tokens JWT para proteger recursos e controlar acesso a API.</p>
            <a href="/login">Fazer Login &rarr;</a>
        </div>
        <div class="feature-card">
            <div class="feature-icon">&#9881;</div>
            <h3>API Version</h3>
            <p>Informacoes sobre a versao atual da API, plataforma e ambiente de execucao.</p>
            <a href="/api/version">Ver Versao &rarr;</a>
        </div>
    </section>

    <footer class="footer">
        <p>&copy; 2026 Tootega. Todos os direitos reservados.</p>
        <p style="margin-top: 0.5rem;">
            <a href="/health">Health Check</a> | 
            <a href="/api/version">Version Info</a>
        </p>
    </footer>
</body>
</html>)";
        res.set_content(html, "text/html; charset=utf-8");
    });

    // Health check endpoint
    m_server->Get("/health", [](const httplib::Request & /*req*/, httplib::Response &res) {
        res.set_content(R"({"status": "healthy"})", "application/json");
    });

    // Register API controllers
    Api::AuthController::registerRoutes(*m_server);
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
