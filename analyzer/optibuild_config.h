#ifndef OPTIBUILD_CONFIG_H
#define OPTIBUILD_CONFIG_H

#include <string>
#include <vector>

class OptiBuildConfig {
public:
    std::string projectName = "CppProject";
    std::string projectRoot = ".";
    std::vector<std::string> sourceDirs = {"src", "include"};
    std::string buildDir = "build";
    std::string buildCommand = "cmake --build build";
    std::string testCommand = "ctest --test-dir build --output-on-failure";
    std::vector<std::string> fileExtensions = {".cpp", ".hpp", ".h", ".cc", ".cxx"};
    std::vector<std::string> ignoreDirs = {"build", ".git", "node_modules", ".optibuild"};

    bool load(const std::string& path);
    bool save(const std::string& path) const;

    static OptiBuildConfig detectDefaults(const std::string& projectRoot);
};

#endif
