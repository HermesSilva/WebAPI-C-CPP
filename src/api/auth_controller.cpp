/**
 * @file auth_controller.cpp
 * @brief Authentication controller implementation
 */

#include "auth_controller.h"
#include "core/jwt.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace Tootega
{
namespace Api
{

void AuthController::registerRoutes(httplib::Server &server)
{
    server.Get("/login", handleLoginPage);
    server.Post("/api/auth/login", handleLogin);
    server.Post("/api/auth/logout", handleLogout);
    server.Get("/api/auth/verify", handleVerify);
    server.Post("/api/auth/refresh", handleRefreshToken);
}

std::string AuthController::extractToken(const httplib::Request &req)
{
    // Check Authorization header
    if (req.has_header("Authorization"))
    {
        std::string auth = req.get_header_value("Authorization");
        if (auth.substr(0, 7) == "Bearer ")
        {
            return auth.substr(7);
        }
    }

    // Check query parameter (for convenience)
    if (req.has_param("token"))
    {
        return req.get_param_value("token");
    }

    return "";
}

bool AuthController::isAuthenticated(const httplib::Request &req)
{
    std::string token = extractToken(req);
    if (token.empty())
    {
        return false;
    }

    Core::JWT::Payload payload;
    return Core::JWT::verifyToken(token, payload);
}

bool AuthController::requiresAuth(const std::string &path)
{
    // Public paths that don't require authentication
    // Note: HTML pages are public, but they check auth via JavaScript
    // API endpoints (/api/*) are protected except auth endpoints
    static const std::vector<std::string> publicPaths = {"/",
                                                         "/health",
                                                         "/login",
                                                         "/browseroso",
                                                         "/api/auth/login",
                                                         "/api/auth/logout",
                                                         "/api/auth/verify",
                                                         "/api/auth/refresh",
                                                         "/api/version",
                                                         "/api/docs"};

    for (const auto &publicPath : publicPaths)
    {
        if (path == publicPath)
        {
            return false;
        }
    }

    return true;
}

bool AuthController::verifyAuth(const httplib::Request &req, httplib::Response &res)
{
    std::string token = extractToken(req);

    if (token.empty())
    {
        res.status = 401;
        res.set_content(R"({"error": "Unauthorized", "message": "No token provided"})", "application/json");
        return false;
    }

    Core::JWT::Payload payload;
    if (!Core::JWT::verifyToken(token, payload))
    {
        res.status = 401;
        res.set_content(R"({"error": "Unauthorized", "message": "Invalid or expired token"})", "application/json");
        return false;
    }

    return true;
}

bool AuthController::verifyAuthWithRedirect(const httplib::Request &req, httplib::Response &res)
{
    std::string token = extractToken(req);

    if (token.empty() || !isAuthenticated(req))
    {
        // Redirect to login page
        res.status = 302;
        res.set_header("Location", "/login");
        res.set_content("", "text/html");
        return false;
    }

    return true;
}

void AuthController::handleLogin(const httplib::Request &req, httplib::Response &res)
{
    // Parse request body for username/password
    std::string username, password;

    // Try to parse JSON body
    std::string body = req.body;

    // Simple JSON parsing for username and password
    auto extractJsonValue = [&body](const std::string &key) -> std::string {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = body.find(searchKey);
        if (pos == std::string::npos)
            return "";

        pos = body.find(':', pos);
        if (pos == std::string::npos)
            return "";
        pos++;

        // Skip whitespace
        while (pos < body.length() && (body[pos] == ' ' || body[pos] == '\t' || body[pos] == '\n'))
            pos++;

        if (pos >= body.length())
            return "";

        if (body[pos] == '"')
        {
            pos++;
            size_t end = body.find('"', pos);
            if (end == std::string::npos)
                return "";
            return body.substr(pos, end - pos);
        }
        return "";
    };

    username = extractJsonValue("username");
    password = extractJsonValue("password");

    // Also try form data
    if (username.empty() && req.has_param("username"))
    {
        username = req.get_param_value("username");
    }
    if (password.empty() && req.has_param("password"))
    {
        password = req.get_param_value("password");
    }

    if (username.empty())
    {
        res.status = 400;
        res.set_content(R"({"error": "Bad Request", "message": "Username is required"})", "application/json");
        return;
    }

    // For now, accept any username/password combination
    // In production, validate against a database

    // Generate JWT token
    std::string token = Core::JWT::createToken(username, username, 86400); // 24 hours

    // Build response
    std::ostringstream json;
    json << "{";
    json << "\"success\": true,";
    json << "\"message\": \"Login successful\",";
    json << "\"token\": \"" << token << "\",";
    json << "\"user\": {";
    json << "\"id\": \"" << username << "\",";
    json << "\"name\": \"" << username << "\"";
    json << "},";
    json << "\"expiresIn\": 86400";
    json << "}";

    res.set_content(json.str(), "application/json");
}

void AuthController::handleLogout(const httplib::Request & /*req*/, httplib::Response &res)
{
    // JWT is stateless, so logout is handled client-side by removing the token
    // This endpoint is provided for API completeness
    res.set_content(R"({"success": true, "message": "Logged out successfully"})", "application/json");
}

void AuthController::handleVerify(const httplib::Request &req, httplib::Response &res)
{
    std::string token = extractToken(req);

    if (token.empty())
    {
        res.status = 401;
        res.set_content(R"({"valid": false, "message": "No token provided"})", "application/json");
        return;
    }

    Core::JWT::Payload payload;
    if (!Core::JWT::verifyToken(token, payload))
    {
        res.status = 401;
        res.set_content(R"({"valid": false, "message": "Invalid or expired token"})", "application/json");
        return;
    }

    std::ostringstream json;
    json << "{";
    json << "\"valid\": true,";
    json << "\"user\": {";
    json << "\"id\": \"" << payload.sub << "\",";
    json << "\"name\": \"" << payload.name << "\"";
    json << "},";
    json << "\"expiresAt\": " << payload.exp;
    json << "}";

    res.set_content(json.str(), "application/json");
}

void AuthController::handleRefreshToken(const httplib::Request &req, httplib::Response &res)
{
    std::string token = extractToken(req);

    if (token.empty())
    {
        res.status = 401;
        res.set_content(R"({"error": "Unauthorized", "message": "No token provided"})", "application/json");
        return;
    }

    Core::JWT::Payload payload;
    if (!Core::JWT::verifyToken(token, payload))
    {
        res.status = 401;
        res.set_content(R"({"error": "Unauthorized", "message": "Invalid token"})", "application/json");
        return;
    }

    // Generate new token
    std::string newToken = Core::JWT::createToken(payload.sub, payload.name, 86400);

    std::ostringstream json;
    json << "{";
    json << "\"success\": true,";
    json << "\"token\": \"" << newToken << "\",";
    json << "\"expiresIn\": 86400";
    json << "}";

    res.set_content(json.str(), "application/json");
}

void AuthController::handleLoginPage(const httplib::Request & /*req*/, httplib::Response &res)
{
    res.set_content(getLoginPageHTML(), "text/html; charset=utf-8");
}

std::string AuthController::getLoginPageHTML()
{
    return R"(<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tootega WebAPI - Login</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }

        .login-container {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 40px;
            width: 100%;
            max-width: 420px;
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.3);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .logo {
            text-align: center;
            margin-bottom: 30px;
        }

        .logo h1 {
            color: #fff;
            font-size: 2em;
            font-weight: 600;
            letter-spacing: 2px;
        }

        .logo span {
            color: #2d7dd2;
        }

        .logo p {
            color: rgba(255, 255, 255, 0.6);
            font-size: 0.9em;
            margin-top: 5px;
        }

        .form-group {
            margin-bottom: 25px;
            position: relative;
        }

        .form-group label {
            display: block;
            color: rgba(255, 255, 255, 0.8);
            font-size: 0.9em;
            margin-bottom: 8px;
            font-weight: 500;
        }

        .form-group input {
            width: 100%;
            padding: 15px 20px;
            background: rgba(255, 255, 255, 0.08);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            color: #fff;
            font-size: 1em;
            transition: all 0.3s ease;
        }

        .form-group input:focus {
            outline: none;
            border-color: #2d7dd2;
            background: rgba(255, 255, 255, 0.12);
            box-shadow: 0 0 20px rgba(45, 125, 210, 0.2);
        }

        .form-group input::placeholder {
            color: rgba(255, 255, 255, 0.4);
        }

        .btn-login {
            width: 100%;
            padding: 16px;
            background: linear-gradient(135deg, #2d7dd2 0%, #1a5fb4 100%);
            border: none;
            border-radius: 12px;
            color: #fff;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .btn-login:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 30px rgba(45, 125, 210, 0.4);
        }

        .btn-login:active {
            transform: translateY(0);
        }

        .btn-login:disabled {
            opacity: 0.7;
            cursor: not-allowed;
            transform: none;
        }

        .message {
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            display: none;
            font-size: 0.9em;
        }

        .message.error {
            background: rgba(220, 53, 69, 0.2);
            border: 1px solid rgba(220, 53, 69, 0.5);
            color: #ff6b6b;
            display: block;
        }

        .message.success {
            background: rgba(40, 167, 69, 0.2);
            border: 1px solid rgba(40, 167, 69, 0.5);
            color: #51cf66;
            display: block;
        }

        .footer {
            text-align: center;
            margin-top: 30px;
            color: rgba(255, 255, 255, 0.4);
            font-size: 0.85em;
        }

        .footer a {
            color: #2d7dd2;
            text-decoration: none;
        }

        .footer a:hover {
            text-decoration: underline;
        }

        .remember-me {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 25px;
        }

        .remember-me input[type="checkbox"] {
            width: 18px;
            height: 18px;
            accent-color: #2d7dd2;
        }

        .remember-me label {
            color: rgba(255, 255, 255, 0.7);
            font-size: 0.9em;
            cursor: pointer;
        }

        .loading {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 2px solid rgba(255,255,255,.3);
            border-radius: 50%;
            border-top-color: #fff;
            animation: spin 0.8s ease-in-out infinite;
            margin-right: 10px;
            vertical-align: middle;
        }

        @keyframes spin {
            to { transform: rotate(360deg); }
        }

        .icon {
            position: absolute;
            right: 15px;
            top: 42px;
            color: rgba(255, 255, 255, 0.4);
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="logo">
            <h1>Tootega<span>API</span></h1>
            <p>Sistema de Gerenciamento</p>
        </div>

        <div id="message" class="message"></div>

        <form id="loginForm">
            <div class="form-group">
                <label for="username">Usuario</label>
                <input type="text" id="username" name="username" placeholder="Digite seu usuario" required autocomplete="username">
                <span class="icon">&#128100;</span>
            </div>

            <div class="form-group">
                <label for="password">Senha</label>
                <input type="password" id="password" name="password" placeholder="Digite sua senha" required autocomplete="current-password">
                <span class="icon">&#128274;</span>
            </div>

            <div class="remember-me">
                <input type="checkbox" id="remember" name="remember">
                <label for="remember">Lembrar-me neste dispositivo</label>
            </div>

            <button type="submit" class="btn-login" id="btnLogin">
                Entrar
            </button>
        </form>

        <div class="footer">
            <p>&copy; 2026 Tootega. Todos os direitos reservados.</p>
            <p style="margin-top: 10px;"><a href="/">Voltar para Home</a> | <a href="/api/docs">Documentacao</a></p>
        </div>
    </div>

    <script>
        const form = document.getElementById('loginForm');
        const message = document.getElementById('message');
        const btnLogin = document.getElementById('btnLogin');

        // Check if already logged in
        const savedToken = localStorage.getItem('jwt_token');
        if (savedToken) {
            verifyExistingToken(savedToken);
        }

        async function verifyExistingToken(token) {
            try {
                const response = await fetch('/api/auth/verify', {
                    headers: { 'Authorization': 'Bearer ' + token }
                });
                
                if (response.ok) {
                    showMessage('Voce ja esta autenticado. Redirecionando...', 'success');
                    setTimeout(() => {
                        window.location.href = '/browseroso';
                    }, 1500);
                } else {
                    localStorage.removeItem('jwt_token');
                }
            } catch (e) {
                localStorage.removeItem('jwt_token');
            }
        }

        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const username = document.getElementById('username').value.trim();
            const password = document.getElementById('password').value;
            const remember = document.getElementById('remember').checked;

            if (!username) {
                showMessage('Por favor, digite seu usuario.', 'error');
                return;
            }

            btnLogin.disabled = true;
            btnLogin.innerHTML = '<span class="loading"></span> Entrando...';

            try {
                const response = await fetch('/api/auth/login', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({ username, password })
                });

                const data = await response.json();

                if (response.ok && data.success) {
                    // Save token
                    if (remember) {
                        localStorage.setItem('jwt_token', data.token);
                    } else {
                        sessionStorage.setItem('jwt_token', data.token);
                    }

                    showMessage('Login realizado com sucesso! Redirecionando...', 'success');
                    
                    setTimeout(() => {
                        window.location.href = '/browseroso';
                    }, 1500);
                } else {
                    showMessage(data.message || 'Erro ao fazer login.', 'error');
                }
            } catch (error) {
                showMessage('Erro de conexao. Tente novamente.', 'error');
                console.error('Login error:', error);
            } finally {
                btnLogin.disabled = false;
                btnLogin.innerHTML = 'Entrar';
            }
        });

        function showMessage(text, type) {
            message.textContent = text;
            message.className = 'message ' + type;
        }
    </script>
</body>
</html>
)";
}

} // namespace Api
} // namespace Tootega
