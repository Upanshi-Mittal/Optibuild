#include "optibuild_cli.h"

#include "dependency_graph.h"
#include "optibuild_config.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {
struct CliOptions {
    std::string command = "scan";
    std::string projectRoot = ".";
    bool dashboard = false;
    bool watch = false;
};

std::string readTextFile(const std::string& path, const std::string& fallback = "{}") {
    std::ifstream in(path);
    if (!in.is_open()) return fallback;
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string quotePath(const std::string& path) {
    std::string quoted = "\"";
    for (char ch : path) {
        if (ch == '"') quoted += "\\\"";
        else quoted += ch;
    }
    quoted += "\"";
    return quoted;
}

std::string absolutePath(const std::string& path) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(path, ec);
    return (ec ? fs::absolute(path) : canonical).lexically_normal().string();
}

std::string configPath(const std::string& root) {
    return (fs::path(root) / ".optibuild" / "config.json").string();
}

std::string cachePath(const std::string& root) {
    return (fs::path(root) / ".optibuild" / "cache.json").string();
}

std::string statusPath(const std::string& root) {
    return (fs::path(root) / ".optibuild" / "status.json").string();
}

std::string dashboardOutputPath(const std::string& root) {
    if (fs::exists("frontend/public")) {
        return "frontend/public/output.json";
    }
    return (fs::path(root) / ".optibuild" / "output.json").string();
}

std::string dashboardGraphPath(const std::string& root) {
    if (fs::exists("frontend/public")) {
        return "frontend/public/graph.json";
    }
    return (fs::path(root) / ".optibuild" / "graph.json").string();
}

void printUsage() {
    std::cout << "OptiBuild reusable C++ selective-build optimizer\n\n";
    std::cout << "Usage:\n";
    std::cout << "  optibuild init [--project path]\n";
    std::cout << "  optibuild scan [--project path]\n";
    std::cout << "  optibuild build [--project path]\n";
    std::cout << "  optibuild test [--project path]\n";
    std::cout << "  optibuild watch [--project path]\n";
    std::cout << "  optibuild dashboard [--project path]\n";
    std::cout << "  optibuild status [--project path]\n";
    std::cout << "  optibuild --project /path/to/project --watch --dashboard\n";
}

CliOptions parseOptions(const std::vector<std::string>& args, const std::string& defaultMode) {
    CliOptions options;
    options.command = defaultMode.empty() || defaultMode.rfind("--", 0) == 0 ? "scan" : defaultMode;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        }
        if (arg == "init" || arg == "scan" || arg == "build" || arg == "test" ||
            arg == "watch" || arg == "dashboard" || arg == "status") {
            options.command = arg;
            continue;
        }
        if (arg == "--project" && i + 1 < args.size()) {
            options.projectRoot = args[++i];
            continue;
        }
        if (arg == "--project=") {
            continue;
        }
        if (arg.rfind("--project=", 0) == 0) {
            options.projectRoot = arg.substr(10);
            continue;
        }
        if (arg == "--watch") {
            options.watch = true;
            options.command = "watch";
            continue;
        }
        if (arg == "--dashboard") {
            options.dashboard = true;
            continue;
        }
        if (arg == "--scan" || arg == "--dashboard-data") {
            options.command = "scan";
            continue;
        }
        if (arg == "--build") {
            options.command = "build";
            continue;
        }
        if (arg == "--test") {
            options.command = "test";
            continue;
        }
    }

    if (options.dashboard && options.watch) {
        options.command = "watch";
    }
    options.projectRoot = absolutePath(options.projectRoot);
    return options;
}

OptiBuildConfig loadOrDetectConfig(const std::string& root) {
    OptiBuildConfig config = OptiBuildConfig::detectDefaults(root);
    config.load(configPath(root));
    if (config.projectRoot == "." || config.projectRoot.empty()) {
        config.projectRoot = root;
    } else if (fs::path(config.projectRoot).is_relative()) {
        config.projectRoot = absolutePath((fs::path(root) / config.projectRoot).string());
    }
    return config;
}

BuildPlan runScan(const OptiBuildConfig& config, bool watchMode, const std::vector<std::string>& extraEvents = {}) {
    const auto started = std::chrono::high_resolution_clock::now();

    DependencyGraph graph(config.projectRoot, config.sourceDirs, config.watchExtensions, config.ignoreDirs);
    graph.loadCache(cachePath(config.projectRoot));
    graph.scan();
    const auto changed = graph.detectChangedFiles();
    graph.findAffectedFiles(changed);

    const auto finished = std::chrono::high_resolution_clock::now();
    const double seconds = std::chrono::duration<double>(finished - started).count();
    auto plan = graph.createBuildPlan(changed, seconds);
    plan.projectName = config.projectName;
    plan.projectRoot = config.projectRoot;
    plan.watchMode = watchMode;

    std::vector<std::string> events = extraEvents;
    for (const auto& file : changed) {
        std::error_code ec;
        auto relative = fs::relative(file, config.projectRoot, ec);
        events.push_back((ec ? file : relative.string()) + " changed");
    }
    events.push_back(std::to_string(plan.affectedFiles.size()) + " affected files detected");

    graph.exportStatusData(plan, statusPath(config.projectRoot), events);
    graph.exportDashboardData(plan, dashboardOutputPath(config.projectRoot), dashboardGraphPath(config.projectRoot));
    graph.saveCache(cachePath(config.projectRoot));
    graph.printSummary(plan);
    return plan;
}

int initProject(const std::string& root) {
    auto config = OptiBuildConfig::detectDefaults(root);
    fs::create_directories(fs::path(root) / ".optibuild");
    config.save(configPath(root));
    std::ofstream(cachePath(root), std::ios::trunc) << "{\n  \"version\": 1,\n  \"files\": [],\n  \"dependencies\": []\n}\n";
    std::ofstream(statusPath(root), std::ios::trunc)
        << "{\n"
        << "  \"projectName\": \"" << config.projectName << "\",\n"
        << "  \"projectRoot\": \"" << config.projectRoot << "\",\n"
        << "  \"lastScanTime\": \"not_scanned\",\n"
        << "  \"watchMode\": false,\n"
        << "  \"totalFiles\": 0,\n"
        << "  \"changedFiles\": [],\n"
        << "  \"affectedFiles\": [],\n"
        << "  \"rebuildFiles\": [],\n"
        << "  \"skippedFiles\": 0,\n"
        << "  \"rebuildReductionPercent\": 0,\n"
        << "  \"buildStatus\": \"not_started\",\n"
        << "  \"testStatus\": \"not_started\",\n"
        << "  \"files\": [],\n"
        << "  \"graph\": { \"edges\": [] },\n"
        << "  \"events\": []\n"
        << "}\n";
    std::cout << "Initialized OptiBuild in " << (fs::path(root) / ".optibuild") << "\n";
    std::cout << "Config: " << configPath(root) << "\n";
    return 0;
}

int runConfiguredCommand(const OptiBuildConfig& config, const std::string& command, const std::string& label) {
    if (command.empty()) {
        std::cout << "No " << label << " command configured.\n";
        return 0;
    }
    std::cout << "Running " << label << ": " << command << "\n";
    const std::string fullCommand = "cd " + quotePath(config.projectRoot) + " && " + command;
    return std::system(fullCommand.c_str());
}

int runBuild(const OptiBuildConfig& config) {
    auto plan = runScan(config, false, {"Build requested"});
    if (plan.rebuildFiles.empty()) {
        std::cout << "No affected source files found, but running configured build for safety.\n";
    }
    return runConfiguredCommand(config, config.buildCommand, "build");
}

int runTest(const OptiBuildConfig& config) {
    runScan(config, false, {"Test requested"});
    return runConfiguredCommand(config, config.testCommand, "test");
}

int runWatch(const OptiBuildConfig& config) {
    std::cout << "Watching " << config.projectRoot << " every 1 second. Press Ctrl+C to stop.\n";
    while (true) {
        runScan(config, true, {"Watch scan complete"});
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string httpResponse(const std::string& body, const std::string& status = "200 OK", const std::string& contentType = "application/json") {
    std::ostringstream out;
    out << "HTTP/1.1 " << status << "\r\n";
    out << "Content-Type: " << contentType << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    out << "Access-Control-Allow-Headers: Content-Type\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n\r\n";
    out << body;
    return out.str();
}

int runApiServer(const OptiBuildConfig& config) {
#ifdef _WIN32
    std::cout << "Dashboard API server is not implemented for Windows in this MVP.\n";
    return 1;
#else
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create API socket.\n";
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config.apiPort);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind API server on port " << config.apiPort << ".\n";
        close(serverFd);
        return 1;
    }
    if (listen(serverFd, 16) < 0) {
        std::cerr << "Failed to listen on API port.\n";
        close(serverFd);
        return 1;
    }

    std::cout << "OptiBuild API running at http://localhost:" << config.apiPort << "\n";
    std::cout << "Start the React dashboard with: cd frontend && npm run dev\n";

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);
        int client = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
        if (client < 0) continue;

        char buffer[4096] = {0};
        read(client, buffer, sizeof(buffer) - 1);
        const std::string request(buffer);

        std::istringstream requestStream(request);
        std::string method;
        std::string path;
        requestStream >> method >> path;

        std::string response;
        if (method == "OPTIONS") {
            response = httpResponse("");
        } else if (method == "GET" && path == "/api/status") {
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else if (method == "GET" && path == "/api/files") {
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else if (method == "GET" && path == "/api/graph") {
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else if (method == "POST" && path == "/api/scan") {
            runScan(config, false, {"API scan requested"});
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else if (method == "POST" && path == "/api/build") {
            runConfiguredCommand(config, config.buildCommand, "build");
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else if (method == "POST" && path == "/api/test") {
            runConfiguredCommand(config, config.testCommand, "test");
            response = httpResponse(readTextFile(statusPath(config.projectRoot)));
        } else {
            response = httpResponse("{\"error\":\"Not found\"}", "404 Not Found");
        }

        send(client, response.c_str(), response.size(), 0);
        close(client);
    }
#endif
}
}

int runOptiBuildCli(const std::vector<std::string>& args, const std::string& defaultMode) {
    const auto options = parseOptions(args, defaultMode);

    if (options.command == "init") {
        return initProject(options.projectRoot);
    }

    auto config = loadOrDetectConfig(options.projectRoot);

    if (options.command == "status") {
        std::cout << readTextFile(statusPath(config.projectRoot), "No OptiBuild status found. Run optibuild scan first.\n");
        return 0;
    }
    if (options.command == "scan") {
        runScan(config, false);
        return 0;
    }
    if (options.command == "build") {
        return runBuild(config);
    }
    if (options.command == "test") {
        return runTest(config);
    }
    if (options.command == "watch") {
        if (options.dashboard) {
            std::thread apiThread([&config]() { runApiServer(config); });
            apiThread.detach();
        }
        return runWatch(config);
    }
    if (options.command == "dashboard") {
        return runApiServer(config);
    }

    printUsage();
    return 1;
}
