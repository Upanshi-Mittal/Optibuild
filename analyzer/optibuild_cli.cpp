#include "optibuild_cli.h"

#include "dependency_graph.h"
#include "optibuild_config.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {
struct CliOptions {
    std::string command = "scan";
    std::string projectRoot = ".";
    std::string outputRoot;
    int intervalSeconds = 1;
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

std::string outputRoot(const CliOptions& options, const std::string& projectRoot) {
    if (!options.outputRoot.empty()) {
        return absolutePath(options.outputRoot);
    }
    return (fs::path(projectRoot) / ".optibuild").string();
}

std::string dashboardOutputPath(const std::string& outputRootPath) {
    return (fs::path(outputRootPath) / "output.json").string();
}

std::string dashboardGraphPath(const std::string& outputRootPath) {
    return (fs::path(outputRootPath) / "graph.json").string();
}

void printUsage() {
    std::cout << "OptiBuild reusable C++ selective-build optimizer\n\n";
    std::cout << "Usage:\n";
    std::cout << "  optibuild --scan [--project path] [--output path]\n";
    std::cout << "  optibuild --watch [--project path] [--output path] [--interval seconds]\n";
    std::cout << "  optibuild --build [--project path] [--output path]\n";
    std::cout << "  optibuild --test [--project path] [--output path]\n";
    std::cout << "  optibuild scan [--project path] [--output path]\n";
    std::cout << "  optibuild watch [--project path] [--output path]\n";
    std::cout << "  optibuild build [--project path] [--output path]\n";
    std::cout << "  optibuild test [--project path] [--output path]\n";
    std::cout << "  optibuild init [--project path]\n";
    std::cout << "  optibuild status [--project path]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  ./optibuild --project /path/to/cpp/project --scan\n";
    std::cout << "  ./optibuild --project /path/to/cpp/project --watch --output frontend/public\n";
    std::cout << "  ./optibuild --project /path/to/cpp/project --scan --output /tmp/optibuild-report\n";
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
        if (arg == "init" || arg == "scan" || arg == "watch" || arg == "build" || arg == "test" || arg == "status") {
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
        if (arg == "--output" && i + 1 < args.size()) {
            options.outputRoot = args[++i];
            continue;
        }
        if (arg.rfind("--output=", 0) == 0) {
            options.outputRoot = arg.substr(9);
            continue;
        }
        if (arg == "--interval" && i + 1 < args.size()) {
            options.intervalSeconds = std::max(1, std::stoi(args[++i]));
            continue;
        }
        if (arg.rfind("--interval=", 0) == 0) {
            options.intervalSeconds = std::max(1, std::stoi(arg.substr(11)));
            continue;
        }
        if (arg == "--scan") {
            options.command = "scan";
            continue;
        }
        if (arg == "--watch") {
            options.command = "watch";
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
        if (arg == "--dashboard" || arg == "dashboard") {
            std::cerr << "Dashboard server mode is not enabled yet. Use --watch --output frontend/public and run the React app separately.\n";
            std::exit(1);
        }
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
    config.save(configPath(root));
    return config;
}

BuildPlan runScan(
    const OptiBuildConfig& config,
    const std::string& outputRootPath,
    const std::vector<std::string>& extraEvents = {},
    bool watchMode = false,
    bool printDetails = true,
    bool saveCacheResult = true
) {
    const auto started = std::chrono::high_resolution_clock::now();

    DependencyGraph graph(config.projectRoot, config.sourceDirs, config.fileExtensions, config.ignoreDirs);
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
    graph.exportDashboardData(plan, dashboardOutputPath(outputRootPath), dashboardGraphPath(outputRootPath));
    if (saveCacheResult) {
        graph.saveCache(cachePath(config.projectRoot));
    }
    if (printDetails) {
        graph.printSummary(plan);
        std::cout << "Config: " << configPath(config.projectRoot) << "\n";
        std::cout << "Cache: " << cachePath(config.projectRoot) << "\n";
        std::cout << "Status: " << statusPath(config.projectRoot) << "\n";
        std::cout << "Dashboard JSON: " << dashboardOutputPath(outputRootPath) << "\n";
    }
    return plan;
}

std::string getJsonString(const std::string& json, const std::string& key, const std::string& fallback = "") {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    return std::regex_search(json, match, pattern) ? match[1].str() : fallback;
}

std::vector<std::string> getJsonStringArray(const std::string& json, const std::string& key) {
    const std::regex arrayPattern("\"" + key + "\"\\s*:\\s*\\[([\\s\\S]*?)\\]");
    std::smatch arrayMatch;
    if (!std::regex_search(json, arrayMatch, arrayPattern)) {
        return {};
    }

    std::vector<std::string> values;
    const std::string body = arrayMatch[1].str();
    const std::regex valuePattern("\"([^\"]*)\"");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), valuePattern);
         it != std::sregex_iterator(); ++it) {
        values.push_back((*it)[1].str());
    }
    return values;
}

void printFileList(const std::string& title, const std::vector<std::string>& files) {
    std::cout << title << " (" << files.size() << ")\n";
    if (files.empty()) {
        std::cout << "  none\n";
        return;
    }
    for (const auto& file : files) {
        std::cout << "  modified: " << file << "\n";
    }
}

void printStatusSummary(const std::string& root) {
    const auto path = statusPath(root);
    if (!fs::exists(path)) {
        std::cout << "No OptiBuild status found. Run optibuild --scan first.\n";
        return;
    }

    const auto json = readTextFile(path);
    const auto changed = getJsonStringArray(json, "changedFiles");
    const auto affected = getJsonStringArray(json, "affectedFiles");
    const auto rebuild = getJsonStringArray(json, "rebuildFiles");

    std::cout << "OptiBuild status\n";
    std::cout << "Project: " << getJsonString(json, "projectName", "unknown") << "\n";
    std::cout << "Root: " << getJsonString(json, "projectRoot", root) << "\n";
    std::cout << "Last scan: " << getJsonString(json, "lastScanTime", "not_scanned") << "\n\n";
    printFileList("Changed files", changed);
    std::cout << "\n";
    printFileList("Affected files", affected);
    std::cout << "\n";
    printFileList("Rebuild candidates", rebuild);
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

int runBuild(const OptiBuildConfig& config, const std::string& outputRootPath) {
    auto plan = runScan(config, outputRootPath, {"Build requested"});
    if (plan.rebuildFiles.empty()) {
        std::cout << "No affected source files found, but running configured build for safety.\n";
    }
    return runConfiguredCommand(config, config.buildCommand, "build");
}

int runTest(const OptiBuildConfig& config, const std::string& outputRootPath) {
    runScan(config, outputRootPath, {"Test requested"});
    return runConfiguredCommand(config, config.testCommand, "test");
}

int runWatch(const OptiBuildConfig& config, const std::string& outputRootPath, int intervalSeconds) {
    std::cout << "Watching " << config.projectRoot << " every " << intervalSeconds << " second(s).\n";
    std::cout << "Press Ctrl+C to stop.\n";
    std::cout << "Dashboard JSON: " << dashboardOutputPath(outputRootPath) << "\n\n";

    if (!fs::exists(cachePath(config.projectRoot))) {
        std::cout << "No cache found. Creating initial baseline first.\n";
        runScan(config, outputRootPath, {"Initial watch baseline created"}, true, false, true);
    }

    while (true) {
        const auto plan = runScan(config, outputRootPath, {"Watch scan complete"}, true, false, false);
        std::cout << "[" << plan.lastScanTime << "] "
                  << plan.changedFiles.size() << " changed, "
                  << plan.affectedFiles.size() << " affected, "
                  << plan.rebuildFiles.size() << " rebuild candidate(s)\n";
        for (const auto& file : plan.changedFiles) {
            std::error_code ec;
            const auto relative = fs::relative(file, config.projectRoot, ec);
            std::cout << "  modified: " << (ec ? file : relative.string()) << "\n";
        }
        std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
}
}

int runOptiBuildCli(const std::vector<std::string>& args, const std::string& defaultMode) {
    const auto options = parseOptions(args, defaultMode);

    if (options.command == "init") {
        return initProject(options.projectRoot);
    }

    auto config = loadOrDetectConfig(options.projectRoot);
    const auto outputRootPath = outputRoot(options, config.projectRoot);

    if (options.command == "status") {
        printStatusSummary(config.projectRoot);
        return 0;
    }
    if (options.command == "scan") {
        runScan(config, outputRootPath);
        return 0;
    }
    if (options.command == "watch") {
        return runWatch(config, outputRootPath, options.intervalSeconds);
    }
    if (options.command == "build") {
        return runBuild(config, outputRootPath);
    }
    if (options.command == "test") {
        return runTest(config, outputRootPath);
    }

    printUsage();
    return 1;
}
