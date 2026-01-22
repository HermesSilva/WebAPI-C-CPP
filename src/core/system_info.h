/**
 * @file system_info.h
 * @brief System information utilities
 */

#pragma once

#include <ctime>
#include <string>

namespace Tootega
{
namespace Core
{

/**
 * @class SystemInfo
 * @brief Provides system information (OS, architecture, etc.)
 */
class SystemInfo
{
  public:
    /**
     * @brief Get singleton instance
     * @return SystemInfo&
     */
    static SystemInfo &getInstance();

    // Disable copy and move
    SystemInfo(const SystemInfo &) = delete;
    SystemInfo &operator=(const SystemInfo &) = delete;
    SystemInfo(SystemInfo &&) = delete;
    SystemInfo &operator=(SystemInfo &&) = delete;

    /**
     * @brief Get OS name
     * @return std::string
     */
    std::string getOSName() const;

    /**
     * @brief Get OS version
     * @return std::string
     */
    std::string getOSVersion() const;

    /**
     * @brief Get CPU architecture
     * @return std::string
     */
    std::string getArchitecture() const;

    /**
     * @brief Get hostname
     * @return std::string
     */
    std::string getHostname() const;

    /**
     * @brief Get API version
     * @return std::string
     */
    std::string getAPIVersion() const;

    /**
     * @brief Get build timestamp
     * @return std::string
     */
    std::string getBuildTimestamp() const;

    /**
     * @brief Get compiler info
     * @return std::string
     */
    std::string getCompilerInfo() const;

    /**
     * @brief Get server uptime in seconds
     * @return long long
     */
    long long getUptimeSeconds() const;

    /**
     * @brief Get formatted uptime string
     * @return std::string
     */
    std::string getUptimeFormatted() const;

  private:
    SystemInfo();
    ~SystemInfo() = default;

    std::time_t m_startTime;
};

} // namespace Core
} // namespace Tootega
