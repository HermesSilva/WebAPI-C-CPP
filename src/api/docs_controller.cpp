/**
 * @file docs_controller.cpp
 * @brief API Documentation controller implementation
 */

#include "docs_controller.h"
#include "core/system_info.h"

#include <sstream>

namespace Tootega
{
namespace Api
{

void DocsController::registerRoutes(httplib::Server &server)
{
    server.Get("/api/docs", getDocsUI);
    server.Get("/api/openapi.json", getOpenAPISpec);
    server.Get("/api/openapi.yaml", getOpenAPISpecYaml);
    server.Get("/api/redoc", getReDoc);
}

void DocsController::getDocsUI(const httplib::Request &req, httplib::Response &res)
{
    std::string host = req.get_header_value("Host");
    if (host.empty())
    {
        host = "localhost:8080";
    }
    std::string specUrl = "http://" + host + "/api/openapi.json";

    res.set_content(generateScalarHTML(specUrl), "text/html; charset=utf-8");
}

void DocsController::getOpenAPISpec(const httplib::Request &req, httplib::Response &res)
{
    std::string host = req.get_header_value("Host");
    if (host.empty())
    {
        host = "localhost:8080";
    }

    res.set_content(generateOpenAPISpec(host), "application/json; charset=utf-8");
}

void DocsController::getOpenAPISpecYaml(const httplib::Request & /*req*/, httplib::Response &res)
{
    auto &sysInfo = Core::SystemInfo::getInstance();

    std::ostringstream yaml;
    yaml << "openapi: '3.0.3'\n";
    yaml << "info:\n";
    yaml << "  title: Tootega WebAPI\n";
    yaml << "  version: '" << sysInfo.getAPIVersion() << "'\n";
    yaml << "servers:\n";
    yaml << "  - url: http://localhost:8080\n";
    yaml << "paths:\n";
    yaml << "  /api/version:\n";
    yaml << "    get:\n";
    yaml << "      summary: Get version info\n";
    yaml << "      responses:\n";
    yaml << "        '200':\n";
    yaml << "          description: Version information\n";

    res.set_content(yaml.str(), "application/x-yaml; charset=utf-8");
}

void DocsController::getReDoc(const httplib::Request &req, httplib::Response &res)
{
    std::string host = req.get_header_value("Host");
    if (host.empty())
    {
        host = "localhost:8080";
    }
    std::string specUrl = "http://" + host + "/api/openapi.json";

    res.set_content(generateReDocHTML(specUrl), "text/html; charset=utf-8");
}

std::string DocsController::generateOpenAPISpec(const std::string &host)
{
    auto &sysInfo = Core::SystemInfo::getInstance();

    std::ostringstream json;
    json << "{\n";
    json << "  \"openapi\": \"3.0.3\",\n";
    json << "  \"info\": {\n";
    json << "    \"title\": \"Tootega WebAPI\",\n";
    json << "    \"description\": \"Cross-platform RESTful API server developed in C++17.\",\n";
    json << "    \"version\": \"" << sysInfo.getAPIVersion() << "\",\n";
    json << "    \"contact\": {\n";
    json << "      \"name\": \"Tootega Development Team\"\n";
    json << "    }\n";
    json << "  },\n";
    json << "  \"servers\": [\n";
    json << "    {\n";
    json << "      \"url\": \"http://" << host << "\",\n";
    json << "      \"description\": \"Current Server\"\n";
    json << "    }\n";
    json << "  ],\n";
    json << "  \"tags\": [\n";
    json << "    { \"name\": \"System\", \"description\": \"System and health endpoints\" },\n";
    json << "    { \"name\": \"Version\", \"description\": \"Version information\" },\n";
    json << "    { \"name\": \"Documentation\", \"description\": \"API documentation\" }\n";
    json << "  ],\n";
    json << "  \"paths\": {\n";

    // Root endpoint
    json << "    \"/\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"System\"],\n";
    json << "        \"summary\": \"Welcome\",\n";
    json << "        \"description\": \"Returns welcome message with API links\",\n";
    json << "        \"operationId\": \"getRoot\",\n";
    json << "        \"responses\": {\n";
    json << "          \"200\": {\n";
    json << "            \"description\": \"Welcome message\",\n";
    json << "            \"content\": {\n";
    json << "              \"application/json\": {\n";
    json << "                \"schema\": { \"$ref\": \"#/components/schemas/WelcomeResponse\" }\n";
    json << "              }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    },\n";

    // Health endpoint
    json << "    \"/health\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"System\"],\n";
    json << "        \"summary\": \"Quick Health Check\",\n";
    json << "        \"description\": \"Simple health check for load balancers\",\n";
    json << "        \"operationId\": \"getHealth\",\n";
    json << "        \"responses\": {\n";
    json << "          \"200\": {\n";
    json << "            \"description\": \"Service is healthy\",\n";
    json << "            \"content\": {\n";
    json << "              \"application/json\": {\n";
    json << "                \"schema\": { \"$ref\": \"#/components/schemas/HealthResponse\" }\n";
    json << "              }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    },\n";

    // Version endpoint
    json << "    \"/api/version\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"Version\"],\n";
    json << "        \"summary\": \"Get Version\",\n";
    json << "        \"description\": \"Returns basic version information\",\n";
    json << "        \"operationId\": \"getVersion\",\n";
    json << "        \"responses\": {\n";
    json << "          \"200\": {\n";
    json << "            \"description\": \"Version information\",\n";
    json << "            \"content\": {\n";
    json << "              \"application/json\": {\n";
    json << "                \"schema\": { \"$ref\": \"#/components/schemas/VersionResponse\" }\n";
    json << "              }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    },\n";

    // Version detailed endpoint
    json << "    \"/api/version/detailed\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"Version\"],\n";
    json << "        \"summary\": \"Get Detailed Version\",\n";
    json << "        \"description\": \"Returns comprehensive version and build information\",\n";
    json << "        \"operationId\": \"getVersionDetailed\",\n";
    json << "        \"responses\": {\n";
    json << "          \"200\": {\n";
    json << "            \"description\": \"Detailed version information\",\n";
    json << "            \"content\": {\n";
    json << "              \"application/json\": {\n";
    json << "                \"schema\": { \"$ref\": \"#/components/schemas/DetailedVersionResponse\" }\n";
    json << "              }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    },\n";

    // API Health endpoint
    json << "    \"/api/health\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"System\"],\n";
    json << "        \"summary\": \"Detailed Health Check\",\n";
    json << "        \"description\": \"Returns detailed health status with uptime\",\n";
    json << "        \"operationId\": \"getDetailedHealth\",\n";
    json << "        \"responses\": {\n";
    json << "          \"200\": {\n";
    json << "            \"description\": \"Detailed health status\",\n";
    json << "            \"content\": {\n";
    json << "              \"application/json\": {\n";
    json << "                \"schema\": { \"$ref\": \"#/components/schemas/DetailedHealthResponse\" }\n";
    json << "              }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    },\n";

    // Docs endpoints
    json << "    \"/api/docs\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"Documentation\"],\n";
    json << "        \"summary\": \"Scalar Documentation\",\n";
    json << "        \"description\": \"Interactive API documentation with Scalar UI\",\n";
    json << "        \"operationId\": \"getDocsScalar\",\n";
    json << "        \"responses\": { \"200\": { \"description\": \"HTML page\" } }\n";
    json << "      }\n";
    json << "    },\n";

    json << "    \"/api/redoc\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"Documentation\"],\n";
    json << "        \"summary\": \"ReDoc\",\n";
    json << "        \"description\": \"Clean ReDoc documentation\",\n";
    json << "        \"operationId\": \"getDocsRedoc\",\n";
    json << "        \"responses\": { \"200\": { \"description\": \"HTML page\" } }\n";
    json << "      }\n";
    json << "    },\n";

    json << "    \"/api/openapi.json\": {\n";
    json << "      \"get\": {\n";
    json << "        \"tags\": [\"Documentation\"],\n";
    json << "        \"summary\": \"OpenAPI JSON\",\n";
    json << "        \"description\": \"Raw OpenAPI 3.0 specification\",\n";
    json << "        \"operationId\": \"getOpenAPIJson\",\n";
    json << "        \"responses\": { \"200\": { \"description\": \"OpenAPI specification\" } }\n";
    json << "      }\n";
    json << "    }\n";

    json << "  },\n";

    // Components/Schemas
    json << "  \"components\": {\n";
    json << "    \"schemas\": {\n";

    json << "      \"WelcomeResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"message\": { \"type\": \"string\" },\n";
    json << "          \"documentation\": { \"type\": \"string\" },\n";
    json << "          \"version\": { \"type\": \"string\" }\n";
    json << "        }\n";
    json << "      },\n";

    json << "      \"HealthResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"status\": { \"type\": \"string\", \"enum\": [\"healthy\", \"unhealthy\"] }\n";
    json << "        }\n";
    json << "      },\n";

    json << "      \"DetailedHealthResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"status\": { \"type\": \"string\" },\n";
    json << "          \"version\": { \"type\": \"string\" },\n";
    json << "          \"uptime_seconds\": { \"type\": \"integer\" },\n";
    json << "          \"timestamp\": { \"type\": \"string\", \"format\": \"date-time\" }\n";
    json << "        }\n";
    json << "      },\n";

    json << "      \"VersionResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"api_version\": { \"type\": \"string\" },\n";
    json << "          \"os\": { \"type\": \"string\" },\n";
    json << "          \"os_version\": { \"type\": \"string\" },\n";
    json << "          \"architecture\": { \"type\": \"string\", \"enum\": [\"x64\", \"ARM64\", \"x86\"] },\n";
    json << "          \"hostname\": { \"type\": \"string\" }\n";
    json << "        }\n";
    json << "      },\n";

    json << "      \"DetailedVersionResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"api\": {\n";
    json << "            \"type\": \"object\",\n";
    json << "            \"properties\": {\n";
    json << "              \"name\": { \"type\": \"string\" },\n";
    json << "              \"version\": { \"type\": \"string\" },\n";
    json << "              \"build_timestamp\": { \"type\": \"string\" }\n";
    json << "            }\n";
    json << "          },\n";
    json << "          \"system\": {\n";
    json << "            \"type\": \"object\",\n";
    json << "            \"properties\": {\n";
    json << "              \"os\": { \"type\": \"string\" },\n";
    json << "              \"os_version\": { \"type\": \"string\" },\n";
    json << "              \"architecture\": { \"type\": \"string\" },\n";
    json << "              \"hostname\": { \"type\": \"string\" }\n";
    json << "            }\n";
    json << "          },\n";
    json << "          \"build\": {\n";
    json << "            \"type\": \"object\",\n";
    json << "            \"properties\": {\n";
    json << "              \"compiler\": { \"type\": \"string\" },\n";
    json << "              \"configuration\": { \"type\": \"string\" },\n";
    json << "              \"target_arch\": { \"type\": \"string\" }\n";
    json << "            }\n";
    json << "          },\n";
    json << "          \"runtime\": {\n";
    json << "            \"type\": \"object\",\n";
    json << "            \"properties\": {\n";
    json << "              \"uptime_seconds\": { \"type\": \"integer\" },\n";
    json << "              \"uptime_formatted\": { \"type\": \"string\" },\n";
    json << "              \"current_time\": { \"type\": \"string\", \"format\": \"date-time\" }\n";
    json << "            }\n";
    json << "          }\n";
    json << "        }\n";
    json << "      },\n";

    json << "      \"ErrorResponse\": {\n";
    json << "        \"type\": \"object\",\n";
    json << "        \"properties\": {\n";
    json << "          \"error\": { \"type\": \"string\" },\n";
    json << "          \"message\": { \"type\": \"string\" },\n";
    json << "          \"path\": { \"type\": \"string\" },\n";
    json << "          \"status\": { \"type\": \"integer\" }\n";
    json << "        }\n";
    json << "      }\n";

    json << "    }\n";
    json << "  }\n";
    json << "}\n";

    return json.str();
}

std::string DocsController::generateScalarHTML(const std::string &specUrl)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>Tootega WebAPI - Documentation</title>\n";
    html << "    <style>\n";
    html << "        body { margin: 0; padding: 0; }\n";
    html << "        .nav-links {\n";
    html << "            position: fixed; top: 10px; right: 20px; z-index: 1000;\n";
    html << "            display: flex; gap: 10px;\n";
    html << "        }\n";
    html << "        .nav-links a {\n";
    html << "            padding: 8px 16px; background: #1a1a2e; color: #fff;\n";
    html << "            text-decoration: none; border-radius: 6px;\n";
    html << "            font-family: system-ui, sans-serif; font-size: 13px;\n";
    html << "        }\n";
    html << "        .nav-links a:hover { background: #16213e; }\n";
    html << "        .nav-links a.active { background: #0f3460; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"nav-links\">\n";
    html << "        <a href=\"/api/docs\" class=\"active\">Scalar</a>\n";
    html << "        <a href=\"/api/redoc\">ReDoc</a>\n";
    html << "        <a href=\"/api/openapi.json\" target=\"_blank\">JSON</a>\n";
    html << "    </div>\n";
    html << "    <script id=\"api-reference\" data-url=\"" << specUrl << "\"></script>\n";
    html << "    <script>\n";
    html << "        var configuration = {\n";
    html << "            theme: 'purple',\n";
    html << "            layout: 'modern',\n";
    html << "            showSidebar: true,\n";
    html << "            hideDarkModeToggle: false,\n";
    html << "            forceDarkModeState: 'dark',\n";
    html << "            defaultOpenAllTags: true\n";
    html << "        }\n";
    html << "    </script>\n";
    html << "    <script src=\"https://cdn.jsdelivr.net/npm/@scalar/api-reference\"></script>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string DocsController::generateReDocHTML(const std::string &specUrl)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>Tootega WebAPI - ReDoc</title>\n";
    html << "    <style>\n";
    html << "        body { margin: 0; padding: 0; }\n";
    html << "        .nav-links {\n";
    html << "            position: fixed; top: 10px; right: 20px; z-index: 1000;\n";
    html << "            display: flex; gap: 10px;\n";
    html << "        }\n";
    html << "        .nav-links a {\n";
    html << "            padding: 8px 16px; background: #1a1a2e; color: #fff;\n";
    html << "            text-decoration: none; border-radius: 6px;\n";
    html << "            font-family: system-ui, sans-serif; font-size: 13px;\n";
    html << "        }\n";
    html << "        .nav-links a:hover { background: #16213e; }\n";
    html << "        .nav-links a.active { background: #0f3460; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"nav-links\">\n";
    html << "        <a href=\"/api/docs\">Scalar</a>\n";
    html << "        <a href=\"/api/redoc\" class=\"active\">ReDoc</a>\n";
    html << "        <a href=\"/api/openapi.json\" target=\"_blank\">JSON</a>\n";
    html << "    </div>\n";
    html << "    <redoc spec-url='" << specUrl << "'></redoc>\n";
    html << "    <script src=\"https://cdn.redoc.ly/redoc/latest/bundles/redoc.standalone.js\"></script>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

} // namespace Api
} // namespace Tootega
