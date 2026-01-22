/**
 * @file main.cpp
 * @brief Entry point for Tootega WebAPI Server
 * @version 1.0.0
 *
 * Cross-platform WebAPI server supporting:
 * - Windows (x64, ARM64)
 * - Linux (x64, ARM64)
 * - WSL
 */

#include <atomic>
#include <csignal>
#include <iostream>

#include "core/server.h"
#include "core/system_info.h"

namespace
{
std::atomic<bool> g_running{true};
Tootega::Core::Server *g_server = nullptr;
} // namespace

void signalHandler(int signum)
{
    std::cout << "\n[INFO] Received signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
    if (g_server)
    {
        g_server->stop();
    }
}

void printBanner()
{
    std::cout << R"(
  _____            _                    
 |_   _|___   ___ | |_ ___  __ _  __ _ 
   | | / _ \ / _ \| __/ _ \/ _` |/ _` |
   | || (_) | (_) | ||  __/ (_| | (_| |
   |_| \___/ \___/ \__\___|\__, |\__,_|
                          |___/        
    )" << std::endl;
    std::cout << "  WebAPI Server v" << API_VERSION << std::endl;
    std::cout << "  ================================" << std::endl;
}

int main(int argc, char *argv[])
{
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef PLATFORM_WINDOWS
    std::signal(SIGBREAK, signalHandler);
#endif

    printBanner();

    // Print system information
    auto &sysInfo = Tootega::Core::SystemInfo::getInstance();
    std::cout << "\n[INFO] System Information:" << std::endl;
    std::cout << "  - OS: " << sysInfo.getOSName() << " " << sysInfo.getOSVersion() << std::endl;
    std::cout << "  - Architecture: " << sysInfo.getArchitecture() << std::endl;
    std::cout << "  - Hostname: " << sysInfo.getHostname() << std::endl;

    // Parse command line arguments
    std::string host = "0.0.0.0";
    int port = 8080;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-h" || arg == "--host") && i + 1 < argc)
        {
            host = argv[++i];
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc)
        {
            port = std::stoi(argv[++i]);
        }
        else if (arg == "--help")
        {
            std::cout << "\nUsage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -h, --host <address>  Bind address (default: 0.0.0.0)\n"
                      << "  -p, --port <port>     Port number (default: 8080)\n"
                      << "  --help                Show this help message\n"
                      << std::endl;
            return 0;
        }
    }

    // Create and start server
    try
    {
        Tootega::Core::Server server(host, port);
        g_server = &server;

        std::cout << "\n[INFO] Starting server on " << host << ":" << port << std::endl;
        std::cout << "[INFO] Press Ctrl+C to stop the server\n" << std::endl;

        if (!server.start())
        {
            std::cerr << "[ERROR] Failed to start server" << std::endl;
            return 1;
        }

        // Server runs until stopped by signal
        g_server = nullptr;
        std::cout << "[INFO] Server stopped gracefully" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
