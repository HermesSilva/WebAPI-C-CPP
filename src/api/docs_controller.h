/**
 * @file docs_controller.h
 * @brief API Documentation controller (Scalar + OpenAPI 3.0)
 */

#pragma once

#include <httplib.h>
#include <string>

namespace Tootega
{
namespace Api
{

/**
 * @class DocsController
 * @brief Handles API documentation endpoints with Scalar UI and OpenAPI 3.0
 */
class DocsController
{
  public:
    /**
     * @brief Register all documentation routes
     * @param server HTTP server instance
     */
    static void registerRoutes(httplib::Server &server);

  private:
    /**
     * @brief GET /api/docs - Scalar UI documentation page
     */
    static void getDocsUI(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief GET /api/openapi.json - OpenAPI 3.0 specification
     */
    static void getOpenAPISpec(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief GET /api/openapi.yaml - OpenAPI 3.0 specification in YAML
     */
    static void getOpenAPISpecYaml(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief GET /api/redoc - ReDoc alternative documentation
     */
    static void getReDoc(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief Generate the OpenAPI 3.0 specification JSON
     * @param host The server host for the spec
     * @return OpenAPI JSON string
     */
    static std::string generateOpenAPISpec(const std::string &host);

    /**
     * @brief Generate the Scalar HTML page
     * @param specUrl URL to the OpenAPI spec
     * @return HTML string
     */
    static std::string generateScalarHTML(const std::string &specUrl);

    /**
     * @brief Generate the ReDoc HTML page
     * @param specUrl URL to the OpenAPI spec
     * @return HTML string
     */
    static std::string generateReDocHTML(const std::string &specUrl);
};

} // namespace Api
} // namespace Tootega
