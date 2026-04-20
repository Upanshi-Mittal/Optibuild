#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <unordered_map>

using namespace std;

// 🌐 Global graph
unordered_map<string, vector<string>> graph;

// 🔁 Topological Sort
vector<string> topoSort(unordered_map<string, vector<string>>& graph) {
    unordered_map<string, int> indegree;

    for (auto& [u, deps] : graph) {
        if (!indegree.count(u)) indegree[u] = 0;
        for (auto& v : deps) {
            indegree[v]++;
        }
    }

    queue<string> q;

    for (auto& [node, deg] : indegree) {
        if (deg == 0) q.push(node);
    }

    vector<string> order;

    while (!q.empty()) {
        string u = q.front();
        q.pop();
        order.push_back(u);

        for (auto& v : graph[u]) {
            if (--indegree[v] == 0) {
                q.push(v);
            }
        }
    }

    return order;
}

// 📂 Load graph
void loadGraph(const string& filename) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        size_t pos = line.find("|");
        if (pos == string::npos) continue;

        string fileNode = line.substr(0, pos);

        size_t start = pos;
        while (start != string::npos) {
            size_t next = line.find("|", start + 1);

            string dep = line.substr(start + 1, next - start - 1);
            graph[fileNode].push_back(dep);

            start = next;
        }
    }
}

int main() {
    cout << "\n=== Selective Build ===\n";

    // 🔥 Load graph
    loadGraph("dependency_graph.cache");

    // 📄 Read impacted files
    vector<string> impacted;
    ifstream in("impacted_files.txt");
    string line;

    while (getline(in, line)) {
        impacted.push_back(line);
    }

    // 🔁 Topological order
    auto order = topoSort(graph);

    // ⚡ Build only impacted files
    for (auto& file : order) {

        // 🔥 FIXED matching (important)
        bool isImpacted = false;

        size_t pos = file.find("src/");
        if (pos == string::npos) continue;

        string shortPath = file.substr(pos); // src/...

        for (auto& f : impacted) {
            if (f.find(shortPath) != string::npos) {
                isImpacted = true;
                break;
            }
        }

        if (!isImpacted) continue;

        if (file.find(".cpp") == string::npos) continue;

        string relative = shortPath;

        cout << "✔ Building: " << relative << endl;

        string cmd = "make CMakeFiles/main.dir/" + relative + ".o";
        system(cmd.c_str());
    }

    return 0;
}