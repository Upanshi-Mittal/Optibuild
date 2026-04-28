#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

using namespace std;

unordered_map<string, vector<string>> graph;

// 🔁 Topo Sort
vector<string> topoSort(unordered_map<string, vector<string>>& graph) {
    unordered_map<string, int> indegree;

    for (auto& [u, deps] : graph) {
        if (!indegree.count(u)) indegree[u] = 0;
        for (auto& v : deps) indegree[v]++;
    }

    queue<string> q;
    for (auto& [node, deg] : indegree)
        if (deg == 0) q.push(node);

    vector<string> order;

    while (!q.empty()) {
        string u = q.front(); q.pop();
        order.push_back(u);

        for (auto& v : graph[u]) {
            if (--indegree[v] == 0)
                q.push(v);
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

    loadGraph("dependency_graph.cache");

    // 📄 impacted files
    vector<string> impacted;
    ifstream in("impacted_files.txt");
    string line;

    while (getline(in, line)) {
        impacted.push_back(line);
    }

    unordered_set<string> impactedSet(impacted.begin(), impacted.end());

    auto order = topoSort(graph);

    int totalFiles = 0;
    int builtFiles = 0;

    // ⏱ start timer
    auto start = chrono::high_resolution_clock::now();

    for (auto& file : order) {

        if (file.find(".cpp") != string::npos)
            totalFiles++;

        size_t pos = file.find("src/");
        if (pos == string::npos) continue;

        string shortPath = file.substr(pos);

        bool isImpacted = false;

        for (auto& f : impacted) {
            if (f.find(shortPath) != string::npos) {
                isImpacted = true;
                break;
            }
        }

        if (!isImpacted) continue;
        if (file.find(".cpp") == string::npos) continue;

        cout << "✔ Building: " << shortPath << endl;

        string cmd = "make CMakeFiles/main.dir/" + shortPath + ".o";
        system(cmd.c_str());

        builtFiles++;
    }

    // ⏱ end timer
    auto end = chrono::high_resolution_clock::now();
    double timeTaken = chrono::duration<double>(end - start).count();

    double optimization = 0;
    if (totalFiles > 0)
        optimization = 100.0 * (totalFiles - builtFiles) / totalFiles;

    // =========================
    // 📊 OUTPUT.JSON
    // =========================

    ofstream json("../frontend/public/output.json");

    json << "{\n";
    json << "  \"built_files\": [\n";

    bool first = true;

    for (auto& file : order) {
        size_t pos = file.find("src/");
        if (pos == string::npos) continue;

        string shortPath = file.substr(pos);

        for (auto& f : impacted) {
            if (f.find(shortPath) != string::npos && file.find(".cpp") != string::npos) {
                if (!first) json << ",\n";
                json << "    \"" << shortPath << "\"";
                first = false;
            }
        }
    }

    json << "\n  ],\n";
    json << "  \"total_files\": " << totalFiles << ",\n";
    json << "  \"built_count\": " << builtFiles << ",\n";
    json << "  \"optimization\": " << optimization << ",\n";
    json << "  \"time_taken\": " << timeTaken << "\n";
    json << "}\n";

    json.close();

    //  GRAPH.JSON

    ofstream graphJson("../frontend/public/graph.json");

    graphJson << "{\n  \"edges\": [\n";

    first = true;

    for (auto& [file, deps] : graph) {
        size_t pos = file.find("src/");
        if (pos == string::npos) continue;

        string from = file.substr(pos);

        for (auto& dep : deps) {
            size_t dpos = dep.find("src/");
            if (dpos == string::npos) continue;

            string to = dep.substr(dpos);

            if (!first) graphJson << ",\n";

            graphJson << "    { \"from\": \"" << from << "\", \"to\": \"" << to << "\" }";
            first = false;
        }
    }

    graphJson << "\n  ]\n}\n";
    graphJson.close();

    return 0;
}