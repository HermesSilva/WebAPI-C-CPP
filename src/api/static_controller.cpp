/**
 * @file static_controller.cpp
 * @brief Static file serving controller implementation
 */

#include "static_controller.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace Tootega
{
namespace Api
{

// Get the directory where the executable is located
std::string getExecutableDirectory()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : path;
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        std::string path(buffer);
        size_t pos = path.find_last_of('/');
        return (pos != std::string::npos) ? path.substr(0, pos) : path;
    }
    return ".";
#endif
}

// Default base path - will be set to executable directory + /web
std::string StaticController::s_basePath = "";

void StaticController::setBasePath(const std::string &path)
{
    s_basePath = path;
}

std::string StaticController::getBasePath()
{
    // Initialize base path on first access if not set
    if (s_basePath.empty())
    {
        s_basePath = getExecutableDirectory() + "/web";
    }
    return s_basePath;
}

void StaticController::registerRoutes(httplib::Server &server)
{
    // Ensure base path is initialized
    getBasePath();

    // HTML Pages
    server.Get("/", serveHomePage);
    server.Get("/login", serveLoginPage);
    server.Get("/browseroso", serveBrowserPage);

    // Static files (CSS, JS, images)
    server.Get(R"(/static/(.*))", serveStaticFile);
}

void StaticController::serveHomePage(const httplib::Request & /*req*/, httplib::Response &res)
{
    std::string content = readFile(getBasePath() + "/pages/home.html");
    if (content.empty())
    {
        res.status = 404;
        res.set_content("Page not found", "text/plain");
        return;
    }
    res.set_content(content, "text/html; charset=utf-8");
}

void StaticController::serveLoginPage(const httplib::Request & /*req*/, httplib::Response &res)
{
    std::string content = readFile(getBasePath() + "/pages/login.html");
    if (content.empty())
    {
        res.status = 404;
        res.set_content("Page not found", "text/plain");
        return;
    }
    res.set_content(content, "text/html; charset=utf-8");
}

void StaticController::serveBrowserPage(const httplib::Request & /*req*/, httplib::Response &res)
{
    std::string content = readFile(getBasePath() + "/pages/browser.html");
    if (content.empty())
    {
        res.status = 404;
        res.set_content("Page not found", "text/plain");
        return;
    }
    res.set_content(content, "text/html; charset=utf-8");
}

void StaticController::serveStaticFile(const httplib::Request &req, httplib::Response &res)
{
    // Get the file path from the URL
    std::string filePath = req.matches[1].str();

    // Security: prevent directory traversal
    if (filePath.find("..") != std::string::npos)
    {
        res.status = 403;
        res.set_content("Forbidden", "text/plain");
        return;
    }

    // Build full path
    std::string fullPath = getBasePath() + "/" + filePath;

    // Check if file exists and read it
    std::string content = readFile(fullPath);
    if (content.empty() && !fileExists(fullPath))
    {
        res.status = 404;
        res.set_content("File not found: " + filePath, "text/plain");
        return;
    }

    // Set content type and cache headers
    std::string mimeType = getMimeType(fullPath);
    res.set_header("Cache-Control", "public, max-age=3600");
    res.set_content(content, mimeType);
}

std::string StaticController::readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool StaticController::fileExists(const std::string &path)
{
    std::ifstream file(path);
    return file.good();
}

std::string StaticController::getMimeType(const std::string &path)
{
    // Extract extension
    size_t dotPos = path.rfind('.');
    if (dotPos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dotPos + 1);

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // MIME type mapping
    if (ext == "html" || ext == "htm")
        return "text/html; charset=utf-8";
    if (ext == "css")
        return "text/css; charset=utf-8";
    if (ext == "js")
        return "application/javascript; charset=utf-8";
    if (ext == "json")
        return "application/json; charset=utf-8";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "ico")
        return "image/x-icon";
    if (ext == "woff")
        return "font/woff";
    if (ext == "woff2")
        return "font/woff2";
    if (ext == "ttf")
        return "font/ttf";
    if (ext == "eot")
        return "application/vnd.ms-fontobject";
    if (ext == "map")
        return "application/json";
    if (ext == "txt")
        return "text/plain; charset=utf-8";
    if (ext == "xml")
        return "application/xml; charset=utf-8";

    return "application/octet-stream";
}

} // namespace Api
} // namespace Tootega
