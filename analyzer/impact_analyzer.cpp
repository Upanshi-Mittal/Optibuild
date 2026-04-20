#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <filesystem>
namespace fs = std::filesystem;
using namespace std;

unordered_map<string, vector<string>> graph;
unordered_map<string, vector<string>> reverseGraph;

// Load graph from cache
void loadGraph(const string& filename) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        size_t pos = line.find("|");
string fileNode = fs::weakly_canonical(line.substr(0, pos)).string();
        size_t start = pos;
        while (start != string::npos) {
            size_t next = line.find("|", start + 1);
string dep = fs::weakly_canonical(line.substr(start + 1, next - start - 1)).string();
            graph[fileNode].push_back(dep);
            reverseGraph[dep].push_back(fileNode);

            start = next;
        }
    }
}

// BFS traversal
vector<string> findImpacted(const vector<string>& changedFiles) {
    vector<string> result;
    unordered_map<string, bool> visited;
    queue<string> q;

    for (auto& f : changedFiles) {
        q.push(f);
        visited[f] = true;
    }

    while (!q.empty()) {
        string curr = q.front();
        q.pop();
        result.push_back(curr);

        for (auto& dep : reverseGraph[curr]) {
            if (!visited[dep]) {
                visited[dep] = true;
                q.push(dep);
            }
        }
    }

    return result;
}

int main() {
    loadGraph("dependency_graph.cache");

    // 🔥 simulate change
    vector<string> changed = {
        "/Users/luna/Desktop/optibuild/src/database/database.h"
    };

    auto impacted = findImpacted(changed);
    ofstream out("impacted_files.txt");

cout << "\n=== Impacted Files ===\n";
for (auto& f : impacted) {
    cout << f << endl;
    out << f << endl;
}

out.close();

    return 0;
}