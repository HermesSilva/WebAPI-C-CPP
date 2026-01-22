/**
 * @file version_controller.cpp
 * @brief Version API endpoint implementation
 */

#include "version_controller.h"
#include "core/system_info.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace Tootega
{
namespace Api
{

void VersionController::registerRoutes(httplib::Server &server)
{
    server.Get("/api/version", getVersion);
    server.Get("/api/version/detailed", getVersionDetailed);
    server.Get("/api/health", getHealth);
}

std::string VersionController::escapeJson(const std::string &str)
{
    std::ostringstream oss;
    for (char c : str)
    {
        switch (c)
        {
        case '"':
            oss << "\\\"";
            break;
        case '\\':
            oss << "\\\\";
            break;
        case '\b':
            oss << "\\b";
            break;
        case '\f':
            oss << "\\f";
            break;
        case '\n':
            oss << "\\n";
            break;
        case '\r':
            oss << "\\r";
            break;
        case '\t':
            oss << "\\t";
            break;
        default:
            oss << c;
            break;
        }
    }
    return oss.str();
}

void VersionController::getVersion(const httplib::Request & /*req*/, httplib::Response &res)
{
    auto &sysInfo = Core::SystemInfo::getInstance();

    std::ostringstream json;
    json << "{\n"
         << "    \"api_version\": \"" << escapeJson(sysInfo.getAPIVersion()) << "\",\n"
         << "    \"os\": \"" << escapeJson(sysInfo.getOSName()) << "\",\n"
         << "    \"os_version\": \"" << escapeJson(sysInfo.getOSVersion()) << "\",\n"
         << "    \"architecture\": \"" << escapeJson(sysInfo.getArchitecture()) << "\",\n"
         << "    \"hostname\": \"" << escapeJson(sysInfo.getHostname()) << "\"\n"
         << "}";

    res.set_content(json.str(), "application/json");
}

void VersionController::getVersionDetailed(const httplib::Request & /*req*/, httplib::Response &res)
{
    auto &sysInfo = Core::SystemInfo::getInstance();

    // Get current timestamp in ISO 8601 format
    auto now = std::time(nullptr);
    auto tm = *std::gmtime(&now);
    std::ostringstream timestampStream;
    timestampStream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    std::ostringstream json;
    json << "{\n"
         << "    \"api\": {\n"
         << "        \"name\": \"Tootega WebAPI\",\n"
         << "        \"version\": \"" << escapeJson(sysInfo.getAPIVersion()) << "\",\n"
         << "        \"build_timestamp\": \"" << escapeJson(sysInfo.getBuildTimestamp()) << "\"\n"
         << "    },\n"
         << "    \"system\": {\n"
         << "        \"os\": \"" << escapeJson(sysInfo.getOSName()) << "\",\n"
         << "        \"os_version\": \"" << escapeJson(sysInfo.getOSVersion()) << "\",\n"
         << "        \"architecture\": \"" << escapeJson(sysInfo.getArchitecture()) << "\",\n"
         << "        \"hostname\": \"" << escapeJson(sysInfo.getHostname()) << "\"\n"
         << "    },\n"
         << "    \"build\": {\n"
         << "        \"compiler\": \"" << escapeJson(sysInfo.getCompilerInfo()) << "\",\n"
#ifdef NDEBUG
         << "        \"configuration\": \"Release\",\n"
#else
         << "        \"configuration\": \"Debug\",\n"
#endif
#ifdef ARCH_X64
         << "        \"target_arch\": \"x64\"\n"
#elif defined(ARCH_ARM64)
         << "        \"target_arch\": \"ARM64\"\n"
#else
         << "        \"target_arch\": \"Unknown\"\n"
#endif
         << "    },\n"
         << "    \"runtime\": {\n"
         << "        \"uptime_seconds\": " << sysInfo.getUptimeSeconds() << ",\n"
         << "        \"uptime_formatted\": \"" << escapeJson(sysInfo.getUptimeFormatted()) << "\",\n"
         << "        \"current_time\": \"" << timestampStream.str() << "\"\n"
         << "    }\n"
         << "}";

    res.set_content(json.str(), "application/json");
}

void VersionController::getHealth(const httplib::Request & /*req*/, httplib::Response &res)
{
    auto &sysInfo = Core::SystemInfo::getInstance();

    // Get current timestamp
    auto now = std::time(nullptr);
    auto tm = *std::gmtime(&now);
    std::ostringstream timestampStream;
    timestampStream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    std::ostringstream json;
    json << "{\n"
         << "    \"status\": \"healthy\",\n"
         << "    \"version\": \"" << escapeJson(sysInfo.getAPIVersion()) << "\",\n"
         << "    \"uptime_seconds\": " << sysInfo.getUptimeSeconds() << ",\n"
         << "    \"timestamp\": \"" << timestampStream.str() << "\"\n"
         << "}";

    res.set_content(json.str(), "application/json");
}

} // namespace Api
} // namespace Tootega
