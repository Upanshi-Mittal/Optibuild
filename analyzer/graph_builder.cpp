#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

unordered_map<string, vector<string>> graph;

vector<string> extractIncludes(const string& filepath) {
    vector<string> includes;
    ifstream file(filepath);
    string line;

    while (getline(file, line)) {
        size_t pos = line.find("#include");
        if (pos != string::npos) {
            size_t start = line.find("\"");
            size_t end = line.find("\"", start + 1);
            if (start != string::npos && end != string::npos) {
                string includeFile = line.substr(start + 1, end - start - 1);

                fs::path fullPath = fs::path(filepath).parent_path() / includeFile;
                includes.push_back(fs::weakly_canonical(fullPath).string());
            }
        }
    }
    return includes;
}

// Build graph from src/
void buildGraph(const string& rootDir) {
    for (auto& p : fs::recursive_directory_iterator(rootDir)) {
        if (p.path().extension() == ".cpp" || p.path().extension() == ".h") {
            string file = p.path().string();
            graph[file] = extractIncludes(file);
        }
    }
}

// Print graph
void printGraph() {
    for (auto& [file, deps] : graph) {
        cout << file << " -> ";
        for (auto& dep : deps) {
            cout << dep << ", ";
        }
        cout << endl;
    }
}

// Save graph to file (cache)
void saveGraph(const string& filename) {
    ofstream out(filename);
    for (auto& [file, deps] : graph) {
        out << file;
        for (auto& dep : deps) {
            out << "|" << dep;
        }
        out << endl;
    }
}

int main() {
    string srcPath = "../src";

    buildGraph(srcPath);

    cout << "\n=== Dependency Graph ===\n";
    printGraph();

    saveGraph("dependency_graph.cache");

    cout << "\nGraph saved to dependency_graph.cache\n";

    return 0;
}