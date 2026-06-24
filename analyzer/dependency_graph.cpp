#include "dependency_graph.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {
bool isSourceFile(const fs::path& path) {
    const auto ext = path.extension().string();
    return ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".h" || ext == ".hpp";
}

bool hasExtension(const fs::path& path, const std::vector<std::string>& extensions) {
    const auto ext = path.extension().string();
    return std::find(extensions.begin(), extensions.end(), ext) != extensions.end();
}

bool isIgnoredPath(const fs::path& path, const std::vector<std::string>& ignoreDirs) {
    for (const auto& part : path) {
        const auto item = part.string();
        if (std::find(ignoreDirs.begin(), ignoreDirs.end(), item) != ignoreDirs.end()) {
            return true;
        }
    }
    return false;
}

bool isCompilableSource(const std::string& path) {
    const auto ext = fs::path(path).extension().string();
    return ext == ".cpp" || ext == ".cc" || ext == ".cxx";
}

std::string canonicalPath(const fs::path& path) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(path, ec);
    return (ec ? fs::absolute(path) : canonical).lexically_normal().string();
}

std::time_t toTimeT(const fs::file_time_type& fileTime) {
    const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        fileTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(systemTime);
}

std::string nowIsoTime() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream out;
    out << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string hashFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return "";
    }

    unsigned long long hash = 1469598103934665603ull;
    char ch = 0;
    while (in.get(ch)) {
        hash ^= static_cast<unsigned char>(ch);
        hash *= 1099511628211ull;
    }

    std::ostringstream out;
    out << std::hex << hash;
    return out.str();
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << ch; break;
        }
    }
    return out.str();
}

std::string displayPath(const std::string& path) {
    const auto srcPos = path.find("src/");
    if (srcPos != std::string::npos) {
        return path.substr(srcPos);
    }
    const auto analyzerPos = path.find("analyzer/");
    if (analyzerPos != std::string::npos) {
        return path.substr(analyzerPos);
    }
    return fs::path(path).filename().string();
}

std::string displayPathFromRoot(const std::string& path, const std::string& root) {
    std::error_code ec;
    const auto relative = fs::relative(path, root, ec);
    if (!ec && !relative.empty()) {
        return relative.string();
    }
    return displayPath(path);
}

std::vector<std::string> extractIncludes(const std::string& filePath) {
    std::ifstream file(filePath);
    std::vector<std::string> includes;
    std::unordered_set<std::string> seen;
    std::string line;
    std::regex includePattern(R"INCLUDE(^\s*#\s*include\s*"([^"]+)")INCLUDE");

    while (std::getline(file, line)) {
        std::smatch match;
        if (!std::regex_search(line, match, includePattern)) {
            continue;
        }

        const fs::path includePath = fs::path(filePath).parent_path() / match[1].str();
        const std::string resolved = canonicalPath(includePath);
        if (seen.insert(resolved).second) {
            includes.push_back(resolved);
        }
    }

    return includes;
}
}

DependencyGraph::DependencyGraph(std::string rootDir)
    : projectRoot(canonicalPath(rootDir)),
      sourceDirs({"src"}),
      watchExtensions({".cpp", ".hpp", ".h", ".cc", ".cxx"}),
      ignoreDirs({"build", ".git", "node_modules", ".optibuild"}) {}

DependencyGraph::DependencyGraph(
    std::string rootDir,
    std::vector<std::string> sourceDirectories,
    std::vector<std::string> extensions,
    std::vector<std::string> ignoredDirectories
) : projectRoot(canonicalPath(rootDir)),
    sourceDirs(std::move(sourceDirectories)),
    watchExtensions(std::move(extensions)),
    ignoreDirs(std::move(ignoredDirectories)) {}

void DependencyGraph::addFile(const std::string& filePath) {
    if (fileMetadata.count(filePath)) {
        return;
    }

    std::error_code ec;
    FileInfo info;
    info.path = filePath;
    info.hash = hashFile(filePath);
    info.lastModified = fs::exists(filePath, ec) ? toTimeT(fs::last_write_time(filePath, ec)) : 0;
    fileMetadata[filePath] = info;
    adjacencyList[filePath];
}

void DependencyGraph::addDependency(const std::string& from, const std::string& to) {
    addFile(from);
    addFile(to);

    auto& dependencies = adjacencyList[from];
    if (std::find(dependencies.begin(), dependencies.end(), to) == dependencies.end()) {
        dependencies.push_back(to);
        reverseGraph[to].push_back(from);
        fileMetadata[from].dependencyCount = dependencies.size();
    }
}

void DependencyGraph::scan() {
    adjacencyList.clear();
    reverseGraph.clear();
    fileMetadata.clear();

    if (!fs::exists(projectRoot)) {
        throw std::runtime_error("Project directory not found: " + projectRoot);
    }

    for (const auto& sourceDir : sourceDirs) {
        const fs::path scanRoot = fs::path(projectRoot) / sourceDir;
        if (!fs::exists(scanRoot)) {
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(scanRoot)) {
            if (!entry.is_regular_file() || isIgnoredPath(entry.path(), ignoreDirs) || !hasExtension(entry.path(), watchExtensions)) {
                continue;
            }

            const std::string filePath = canonicalPath(entry.path());
            addFile(filePath);
            for (const auto& dependency : extractIncludes(filePath)) {
                if (fs::exists(dependency) && hasExtension(dependency, watchExtensions) && !isIgnoredPath(dependency, ignoreDirs)) {
                    addDependency(filePath, dependency);
                }
            }
        }
    }
}

void DependencyGraph::loadCache(const std::string& cachePath) {
    cachedHashes.clear();
    std::ifstream in(cachePath);
    if (!in.is_open()) {
        return;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::regex filePattern(R"CACHE(\{\s*"path"\s*:\s*"([^"]+)"\s*,\s*"hash"\s*:\s*"([^"]+)")CACHE");

    for (auto it = std::sregex_iterator(content.begin(), content.end(), filePattern);
         it != std::sregex_iterator(); ++it) {
        cachedHashes[(*it)[1].str()] = (*it)[2].str();
    }
}

void DependencyGraph::saveCache(const std::string& cachePath) const {
    fs::create_directories(fs::path(cachePath).parent_path());
    std::ofstream out(cachePath, std::ios::trunc);

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"generatedAt\": \"" << nowIsoTime() << "\",\n";
    out << "  \"files\": [\n";

    std::size_t index = 0;
    for (const auto& [path, info] : fileMetadata) {
        out << "    { \"path\": \"" << jsonEscape(path) << "\", \"hash\": \"" << info.hash
            << "\", \"lastModified\": " << info.lastModified
            << ", \"dependencyCount\": " << info.dependencyCount << " }";
        if (++index < fileMetadata.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ],\n";
    out << "  \"dependencies\": [\n";

    bool first = true;
    for (const auto& [from, deps] : adjacencyList) {
        for (const auto& to : deps) {
            if (!first) {
                out << ",\n";
            }
            out << "    { \"from\": \"" << jsonEscape(from) << "\", \"to\": \"" << jsonEscape(to) << "\" }";
            first = false;
        }
    }

    out << "\n  ]\n";
    out << "}\n";
}

std::vector<std::string> DependencyGraph::getDependencies(const std::string& filePath) const {
    const auto it = adjacencyList.find(filePath);
    return it == adjacencyList.end() ? std::vector<std::string>{} : it->second;
}

std::vector<std::string> DependencyGraph::getDependents(const std::string& filePath) const {
    const auto it = reverseGraph.find(filePath);
    return it == reverseGraph.end() ? std::vector<std::string>{} : it->second;
}

std::vector<std::string> DependencyGraph::detectChangedFiles() {
    std::vector<std::string> changed;
    for (auto& [path, info] : fileMetadata) {
        const auto cached = cachedHashes.find(path);
        info.changed = cached == cachedHashes.end() || cached->second != info.hash;
        if (info.changed) {
            changed.push_back(path);
        }
    }
    std::sort(changed.begin(), changed.end());
    return changed;
}

std::unordered_set<std::string> DependencyGraph::findAffectedFiles(const std::vector<std::string>& changedFiles) {
    std::unordered_set<std::string> affected;
    std::queue<std::string> queue;

    for (const auto& file : changedFiles) {
        affected.insert(file);
        queue.push(file);
    }

    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();

        for (const auto& dependent : getDependents(current)) {
            if (affected.insert(dependent).second) {
                queue.push(dependent);
            }
        }
    }

    for (auto& [path, info] : fileMetadata) {
        info.affected = affected.count(path) > 0;
    }

    return affected;
}

BuildPlan DependencyGraph::createBuildPlan(const std::vector<std::string>& changedFiles, double scanSeconds) const {
    BuildPlan plan;
    plan.changedFiles = changedFiles;
    plan.scanSeconds = scanSeconds;
    plan.lastScanTime = nowIsoTime();

    for (const auto& [path, info] : fileMetadata) {
        if (isCompilableSource(path)) {
            plan.totalFiles++;
            if (info.affected) {
                plan.rebuildFiles.push_back(path);
            }
        }
        if (info.affected) {
            plan.affectedFiles.push_back(path);
        }
    }

    std::sort(plan.affectedFiles.begin(), plan.affectedFiles.end());
    std::sort(plan.rebuildFiles.begin(), plan.rebuildFiles.end());
    plan.skippedFiles = plan.totalFiles > plan.rebuildFiles.size() ? plan.totalFiles - plan.rebuildFiles.size() : 0;
    if (plan.totalFiles > 0) {
        plan.reductionPercentage = 100.0 * static_cast<double>(plan.skippedFiles) / static_cast<double>(plan.totalFiles);
    }
    return plan;
}

void DependencyGraph::exportDashboardData(const BuildPlan& plan, const std::string& outputPath, const std::string& graphPath) const {
    fs::create_directories(fs::path(outputPath).parent_path());
    fs::create_directories(fs::path(graphPath).parent_path());

    std::ofstream out(outputPath, std::ios::trunc);
    out << "{\n";
    out << "  \"summary\": {\n";
    out << "    \"total_files\": " << fileMetadata.size() << ",\n";
    out << "    \"source_files\": " << plan.totalFiles << ",\n";
    out << "    \"changed_files\": " << plan.changedFiles.size() << ",\n";
    out << "    \"affected_files\": " << plan.affectedFiles.size() << ",\n";
    out << "    \"built_count\": " << plan.rebuildFiles.size() << ",\n";
    out << "    \"skipped_files\": " << plan.skippedFiles << ",\n";
    out << "    \"optimization\": " << std::fixed << std::setprecision(2) << plan.reductionPercentage << ",\n";
    out << "    \"time_taken\": " << std::setprecision(4) << plan.scanSeconds << ",\n";
    out << "    \"last_scan_time\": \"" << jsonEscape(plan.lastScanTime) << "\"\n";
    out << "  },\n";

    out << "  \"built_files\": [";
    for (std::size_t i = 0; i < plan.rebuildFiles.size(); ++i) {
        out << (i == 0 ? "\n" : ",\n") << "    \"" << jsonEscape(displayPathFromRoot(plan.rebuildFiles[i], projectRoot)) << "\"";
    }
    out << (plan.rebuildFiles.empty() ? "" : "\n") << "  ],\n";

    out << "  \"files\": [\n";
    std::size_t index = 0;
    for (const auto& [path, info] : fileMetadata) {
        const std::string status = info.changed ? "changed" : (info.affected ? "affected" : "skipped");
        out << "    { \"path\": \"" << jsonEscape(displayPathFromRoot(path, projectRoot)) << "\", \"status\": \"" << status
            << "\", \"dependencyCount\": " << info.dependencyCount
            << ", \"hash\": \"" << info.hash << "\" }";
        if (++index < fileMetadata.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";

    std::ofstream graphOut(graphPath, std::ios::trunc);
    graphOut << "{\n  \"nodes\": [\n";
    index = 0;
    for (const auto& [path, info] : fileMetadata) {
        const std::string status = info.changed ? "changed" : (info.affected ? "affected" : "skipped");
        graphOut << "    { \"id\": \"" << jsonEscape(displayPathFromRoot(path, projectRoot)) << "\", \"status\": \"" << status << "\" }";
        if (++index < fileMetadata.size()) {
            graphOut << ",";
        }
        graphOut << "\n";
    }
    graphOut << "  ],\n  \"edges\": [\n";

    bool first = true;
    for (const auto& [from, deps] : adjacencyList) {
        for (const auto& to : deps) {
            if (!first) {
                graphOut << ",\n";
            }
            graphOut << "    { \"from\": \"" << jsonEscape(displayPathFromRoot(from, projectRoot))
                     << "\", \"to\": \"" << jsonEscape(displayPathFromRoot(to, projectRoot)) << "\" }";
            first = false;
        }
    }
    graphOut << "\n  ]\n}\n";
}

void DependencyGraph::exportStatusData(const BuildPlan& plan, const std::string& statusPath, const std::vector<std::string>& events) const {
    fs::create_directories(fs::path(statusPath).parent_path());
    std::ofstream out(statusPath, std::ios::trunc);

    auto writeArray = [&](const std::string& key, const std::vector<std::string>& values, bool comma) {
        out << "  \"" << key << "\": [";
        for (std::size_t i = 0; i < values.size(); ++i) {
            out << (i == 0 ? "\n" : ",\n") << "    \"" << jsonEscape(displayPathFromRoot(values[i], projectRoot)) << "\"";
        }
        out << (values.empty() ? "" : "\n") << "  ]";
        if (comma) out << ",";
        out << "\n";
    };

    out << "{\n";
    out << "  \"projectName\": \"" << jsonEscape(plan.projectName) << "\",\n";
    out << "  \"projectRoot\": \"" << jsonEscape(plan.projectRoot.empty() ? projectRoot : plan.projectRoot) << "\",\n";
    out << "  \"lastScanTime\": \"" << jsonEscape(plan.lastScanTime) << "\",\n";
    out << "  \"watchMode\": " << (plan.watchMode ? "true" : "false") << ",\n";
    out << "  \"totalFiles\": " << fileMetadata.size() << ",\n";
    writeArray("changedFiles", plan.changedFiles, true);
    writeArray("affectedFiles", plan.affectedFiles, true);
    writeArray("rebuildFiles", plan.rebuildFiles, true);
    out << "  \"skippedFiles\": " << plan.skippedFiles << ",\n";
    out << "  \"rebuildReductionPercent\": " << std::fixed << std::setprecision(2) << plan.reductionPercentage << ",\n";
    out << "  \"buildStatus\": \"" << jsonEscape(plan.buildStatus) << "\",\n";
    out << "  \"testStatus\": \"" << jsonEscape(plan.testStatus) << "\",\n";
    out << "  \"files\": [\n";
    std::size_t index = 0;
    for (const auto& [path, info] : fileMetadata) {
        const std::string status = info.changed ? "changed" : (info.affected ? "affected" : "skipped");
        out << "    { \"path\": \"" << jsonEscape(displayPathFromRoot(path, projectRoot))
            << "\", \"status\": \"" << status << "\", \"dependencyCount\": " << info.dependencyCount << " }";
        if (++index < fileMetadata.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"graph\": {\n";
    out << "    \"edges\": [\n";
    bool first = true;
    for (const auto& [from, deps] : adjacencyList) {
        for (const auto& to : deps) {
            if (!first) out << ",\n";
            out << "      { \"from\": \"" << jsonEscape(displayPathFromRoot(from, projectRoot))
                << "\", \"to\": \"" << jsonEscape(displayPathFromRoot(to, projectRoot)) << "\" }";
            first = false;
        }
    }
    out << "\n    ]\n";
    out << "  },\n";
    out << "  \"events\": [\n";
    for (std::size_t i = 0; i < events.size(); ++i) {
        out << "    { \"time\": \"" << jsonEscape(plan.lastScanTime)
            << "\", \"type\": \"event\", \"message\": \"" << jsonEscape(events[i]) << "\" }";
        if (i + 1 < events.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

void DependencyGraph::printSummary(const BuildPlan& plan) const {
    std::cout << "\n=== OptiBuild Summary ===\n";
    std::cout << "Files scanned: " << fileMetadata.size() << '\n';
    std::cout << "Changed files: " << plan.changedFiles.size() << '\n';
    std::cout << "Affected files: " << plan.affectedFiles.size() << '\n';
    std::cout << "Rebuild .cpp files: " << plan.rebuildFiles.size() << '\n';
    std::cout << "Skipped .cpp files: " << plan.skippedFiles << '\n';
    std::cout << "Rebuild reduction: " << std::fixed << std::setprecision(2) << plan.reductionPercentage << "%\n";
}

const std::unordered_map<std::string, std::vector<std::string>>& DependencyGraph::dependencies() const {
    return adjacencyList;
}

const std::unordered_map<std::string, FileInfo>& DependencyGraph::files() const {
    return fileMetadata;
}
