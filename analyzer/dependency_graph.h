#ifndef OPTIBUILD_DEPENDENCY_GRAPH_H
#define OPTIBUILD_DEPENDENCY_GRAPH_H

#include <ctime>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct FileInfo {
    std::string path;
    std::string hash;
    std::time_t lastModified = 0;
    std::size_t dependencyCount = 0;
    bool changed = false;
    bool affected = false;
};

struct BuildPlan {
    std::string projectName;
    std::string projectRoot;
    std::vector<std::string> changedFiles;
    std::vector<std::string> affectedFiles;
    std::vector<std::string> rebuildFiles;
    std::size_t totalFiles = 0;
    std::size_t skippedFiles = 0;
    double reductionPercentage = 0.0;
    double scanSeconds = 0.0;
    std::string lastScanTime;
    std::string buildStatus = "not_started";
    std::string testStatus = "not_started";
    bool watchMode = false;
};

class DependencyGraph {
private:
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacencyList;
    std::unordered_map<std::string, std::unordered_set<std::string>> reverseGraph;
    std::unordered_map<std::string, FileInfo> fileMetadata;
    std::unordered_map<std::string, std::string> cachedHashes;

    std::string projectRoot;
    std::vector<std::string> sourceDirs;
    std::vector<std::string> fileExtensions;
    std::vector<std::string> ignoreDirs;

public:
    explicit DependencyGraph(std::string rootDir = ".");
    DependencyGraph(
        std::string rootDir,
        std::vector<std::string> sourceDirectories,
        std::vector<std::string> extensions,
        std::vector<std::string> ignoredDirectories
    );

    void addFile(const std::string& filePath);
    void addDependency(const std::string& from, const std::string& to);
    void scan();
    void loadCache(const std::string& cachePath);
    void saveCache(const std::string& cachePath) const;

    std::vector<std::string> getDependencies(const std::string& filePath) const;
    std::vector<std::string> getDependents(const std::string& filePath) const;
    std::vector<std::string> detectChangedFiles();
    std::unordered_set<std::string> findAffectedFiles(const std::vector<std::string>& changedFiles);
    BuildPlan createBuildPlan(const std::vector<std::string>& changedFiles, double scanSeconds) const;

    void exportDashboardData(const BuildPlan& plan, const std::string& outputPath, const std::string& graphPath) const;
    void exportStatusData(const BuildPlan& plan, const std::string& statusPath, const std::vector<std::string>& events) const;
    void printSummary(const BuildPlan& plan) const;

    const std::unordered_map<std::string, std::unordered_set<std::string>>& dependencies() const;
    const std::unordered_map<std::string, FileInfo>& files() const;
};

#endif
