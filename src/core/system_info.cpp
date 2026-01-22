/**
 * @file system_info.cpp
 * @brief System information implementation
 */

#include "system_info.h"

#include <cstring>
#include <iomanip>
#include <sstream>

#ifdef PLATFORM_WINDOWS
// clang-format off
#include <windows.h>
#include <versionhelpers.h>
// clang-format on
#else
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace Tootega
{
namespace Core
{

SystemInfo::SystemInfo() : m_startTime(std::time(nullptr))
{
}

SystemInfo &SystemInfo::getInstance()
{
    static SystemInfo instance;
    return instance;
}

std::string SystemInfo::getOSName() const
{
#ifdef PLATFORM_WINDOWS
    return "Windows";
#elif defined(PLATFORM_LINUX)
    return "Linux";
#elif defined(PLATFORM_MACOS)
    return "macOS";
#else
    return "Unknown";
#endif
}

std::string SystemInfo::getOSVersion() const
{
#ifdef PLATFORM_WINDOWS
    // Get Windows version info
    OSVERSIONINFOEXW osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    // Use RtlGetVersion to get accurate version info
    typedef LONG(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr)
        {
            fxPtr((PRTL_OSVERSIONINFOW)&osvi);
            std::ostringstream oss;
            oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "." << osvi.dwBuildNumber;
            return oss.str();
        }
    }
    return "Unknown";
#else
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        return std::string(unameData.release);
    }
    return "Unknown";
#endif
}

std::string SystemInfo::getArchitecture() const
{
#ifdef PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    default:
        return "Unknown";
    }
#else
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        std::string machine = unameData.machine;
        if (machine == "x86_64" || machine == "amd64")
        {
            return "x64";
        }
        else if (machine == "aarch64" || machine == "arm64")
        {
            return "ARM64";
        }
        else if (machine.find("arm") != std::string::npos)
        {
            return "ARM";
        }
        else if (machine == "i686" || machine == "i386")
        {
            return "x86";
        }
        return machine;
    }
    return "Unknown";
#endif
}

std::string SystemInfo::getHostname() const
{
#ifdef PLATFORM_WINDOWS
    char hostname[256];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size))
    {
        return std::string(hostname);
    }
    return "Unknown";
#else
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        return std::string(hostname);
    }
    return "Unknown";
#endif
}

std::string SystemInfo::getAPIVersion() const
{
#ifdef API_VERSION
    return API_VERSION;
#else
    return "1.0.0";
#endif
}

std::string SystemInfo::getBuildTimestamp() const
{
    return std::string(__DATE__) + " " + std::string(__TIME__);
}

std::string SystemInfo::getCompilerInfo() const
{
    std::ostringstream oss;

#if defined(_MSC_VER)
    oss << "MSVC " << _MSC_VER;
#if _MSC_VER >= 1930
    oss << " (VS 2022)";
#elif _MSC_VER >= 1920
    oss << " (VS 2019)";
#elif _MSC_VER >= 1910
    oss << " (VS 2017)";
#endif
#elif defined(__clang__)
    oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
    oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
    oss << "Unknown Compiler";
#endif

    return oss.str();
}

long long SystemInfo::getUptimeSeconds() const
{
    return static_cast<long long>(std::difftime(std::time(nullptr), m_startTime));
}

std::string SystemInfo::getUptimeFormatted() const
{
    long long totalSeconds = getUptimeSeconds();

    long long days = totalSeconds / 86400;
    long long hours = (totalSeconds % 86400) / 3600;
    long long minutes = (totalSeconds % 3600) / 60;
    long long seconds = totalSeconds % 60;

    std::ostringstream oss;
    if (days > 0)
    {
        oss << days << "d ";
    }
    oss << std::setfill('0') << std::setw(2) << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return oss.str();
}

} // namespace Core
} // namespace Tootega
