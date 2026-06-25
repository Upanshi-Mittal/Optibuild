#include "optibuild_config.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace {
std::string escapeJson(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        if (ch == '\\') out << "\\\\";
        else if (ch == '"') out << "\\\"";
        else out << ch;
    }
    return out.str();
}

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string getString(const std::string& json, const std::string& key, const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    return std::regex_search(json, match, pattern) ? match[1].str() : fallback;
}

std::vector<std::string> getStringArray(const std::string& json, const std::string& key, const std::vector<std::string>& fallback) {
    const std::regex arrayPattern("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch arrayMatch;
    if (!std::regex_search(json, arrayMatch, arrayPattern)) {
        return fallback;
    }

    std::vector<std::string> values;
    const std::string body = arrayMatch[1].str();
    const std::regex valuePattern("\"([^\"]*)\"");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), valuePattern);
         it != std::sregex_iterator(); ++it) {
        values.push_back((*it)[1].str());
    }
    return values.empty() ? fallback : values;
}

void writeStringArray(std::ofstream& out, const std::string& key, const std::vector<std::string>& values, bool trailingComma) {
    out << "  \"" << key << "\": [";
    for (std::size_t i = 0; i < values.size(); ++i) {
        out << "\"" << escapeJson(values[i]) << "\"";
        if (i + 1 < values.size()) out << ", ";
    }
    out << "]";
    if (trailingComma) out << ",";
    out << "\n";
}
}

bool OptiBuildConfig::load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    const std::string json = readFile(path);
    projectName = getString(json, "projectName", projectName);
    projectRoot = getString(json, "projectRoot", projectRoot);
    sourceDirs = getStringArray(json, "sourceDirs", sourceDirs);
    buildDir = getString(json, "buildDir", buildDir);
    buildCommand = getString(json, "buildCommand", buildCommand);
    testCommand = getString(json, "testCommand", testCommand);
    fileExtensions = getStringArray(json, "fileExtensions", fileExtensions);
    fileExtensions = getStringArray(json, "watchExtensions", fileExtensions);
    ignoreDirs = getStringArray(json, "ignoreDirs", ignoreDirs);
    return true;
}

bool OptiBuildConfig::save(const std::string& path) const {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"projectName\": \"" << escapeJson(projectName) << "\",\n";
    out << "  \"projectRoot\": \"" << escapeJson(projectRoot) << "\",\n";
    writeStringArray(out, "sourceDirs", sourceDirs, true);
    out << "  \"buildDir\": \"" << escapeJson(buildDir) << "\",\n";
    out << "  \"buildCommand\": \"" << escapeJson(buildCommand) << "\",\n";
    out << "  \"testCommand\": \"" << escapeJson(testCommand) << "\",\n";
    writeStringArray(out, "fileExtensions", fileExtensions, true);
    writeStringArray(out, "ignoreDirs", ignoreDirs, false);
    out << "}\n";
    return true;
}

OptiBuildConfig OptiBuildConfig::detectDefaults(const std::string& projectRoot) {
    OptiBuildConfig config;
    std::error_code ec;
    config.projectRoot = fs::weakly_canonical(projectRoot, ec).string();
    if (config.projectRoot.empty()) {
        config.projectRoot = fs::absolute(projectRoot).lexically_normal().string();
    }
    config.projectName = fs::path(config.projectRoot).filename().string();

    config.sourceDirs.clear();
    if (fs::exists(fs::path(config.projectRoot) / "src")) config.sourceDirs.push_back("src");
    if (fs::exists(fs::path(config.projectRoot) / "include")) config.sourceDirs.push_back("include");
    if (config.sourceDirs.empty()) config.sourceDirs.push_back(".");

    if (fs::exists(fs::path(config.projectRoot) / "CMakeLists.txt")) {
        config.buildCommand = "cmake --build build";
        config.testCommand = "ctest --test-dir build --output-on-failure";
    } else if (fs::exists(fs::path(config.projectRoot) / "Makefile")) {
        config.buildCommand = "make";
        config.testCommand = "make test";
    } else {
        config.buildCommand = "echo \"No build system detected. Configure buildCommand in .optibuild/config.json.\"";
        config.testCommand = "echo \"No test command configured.\"";
    }

    return config;
}
