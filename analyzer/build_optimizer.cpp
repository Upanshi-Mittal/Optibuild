#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

using namespace std;

unordered_map<string, vector<string>> graph;

// 🔁 Topological Sort
vector<string> topoSort(unordered_map<string, vector<string>> &graph)
{
    unordered_map<string, int> indegree;

    for (auto &[u, deps] : graph)
    {
        if (!indegree.count(u))
            indegree[u] = 0;
        for (auto &v : deps)
            indegree[v]++;
    }

    queue<string> q;
    for (auto &[node, deg] : indegree)
    {
        if (deg == 0)
            q.push(node);
    }

    vector<string> order;

    while (!q.empty())
    {
        string u = q.front();
        q.pop();
        order.push_back(u);

        for (auto &v : graph[u])
        {
            if (--indegree[v] == 0)
                q.push(v);
        }
    }

    return order;
}

// 📂 Load dependency graph
void loadGraph(const string &filename)
{
    ifstream file(filename);
    string line;

    while (getline(file, line))
    {
        size_t pos = line.find("|");
        if (pos == string::npos)
            continue;

        string fileNode = line.substr(0, pos);

        size_t start = pos;
        while (start != string::npos)
        {
            size_t next = line.find("|", start + 1);
            string dep = line.substr(start + 1, next - start - 1);
            graph[fileNode].push_back(dep);
            start = next;
        }
    }
}

int main()
{

    cout << "\n=== Selective Build ===\n";

    loadGraph("dependency_graph.cache");

    // 📄 Load impacted files
    unordered_set<string> impactedSet;
    ifstream in("impacted_files.txt");
    string line;

    while (getline(in, line))
    {
        if (!line.empty())
            impactedSet.insert(line);
    }

    auto order = topoSort(graph);

    int totalFiles = 0;
    int builtFiles = 0;

    vector<string> builtList;

    // ⏱ Start timer
    auto start = chrono::high_resolution_clock::now();

    for (auto &file : order)
    {

        if (file.find(".cpp") != string::npos)
            totalFiles++;

        size_t pos = file.find("src/");
        if (pos == string::npos)
            continue;

        string shortPath = file.substr(pos);

        bool isImpacted = false;

        for (auto &f : impactedSet)
        {
            if (f == file)
            {
                isImpacted = true;
                break;
            }
        }
        bool isImpacted = impactedSet.count(file);
        if (!isImpacted)
            continue;
        if (file.find(".cpp") == string::npos)
            continue;

        cout << "✔ Building: " << shortPath << endl;

        string cmd = "make CMakeFiles/main.dir/" + shortPath + ".o";
        system(cmd.c_str());

        builtFiles++;
        builtList.push_back(shortPath);
    }

    // ⏱ End timer
    auto end = chrono::high_resolution_clock::now();
    double timeTaken = chrono::duration<double>(end - start).count();

    double optimization = 0;
    if (totalFiles > 0)
        optimization = 100.0 * (totalFiles - builtFiles) / totalFiles;

    // =========================
    // 📊 WRITE OUTPUT.JSON
    // =========================

    string jsonPath = "/Users/luna/Desktop/optibuild/frontend/public/output.json";

    ofstream json(jsonPath, ios::trunc);

    if (!json.is_open())
    {
        cerr << "❌ Failed to open output.json\n";
        return 1;
    }

    json << "{\n";
    json << "  \"built_files\": [\n";

    for (size_t i = 0; i < builtList.size(); i++)
    {
        json << "    \"" << builtList[i] << "\"";
        if (i != builtList.size() - 1)
            json << ",";
        json << "\n";
    }

    json << "  ],\n";
    json << "  \"total_files\": " << totalFiles << ",\n";
    json << "  \"built_count\": " << builtFiles << ",\n";
    json << "  \"optimization\": " << optimization << ",\n";
    json << "  \"time_taken\": " << timeTaken << "\n";
    json << "}\n";

    json.close();

    cout << "\n✅ output.json updated!\n";

    // =========================
    // 🔗 WRITE GRAPH.JSON
    // =========================

    ofstream graphJson("../frontend/public/graph.json", ios::trunc);

    graphJson << "{\n  \"edges\": [\n";

    bool first = true;

    for (auto &[file, deps] : graph)
    {
        size_t pos = file.find("src/");
        if (pos == string::npos)
            continue;

        string from = file.substr(pos);

        for (auto &dep : deps)
        {
            size_t dpos = dep.find("src/");
            if (dpos == string::npos)
                continue;

            string to = dep.substr(dpos);

            if (!first)
                graphJson << ",\n";

            graphJson << "    { \"from\": \"" << from << "\", \"to\": \"" << to << "\" }";
            first = false;
        }
    }

    graphJson << "\n  ]\n}\n";
    graphJson.close();

    cout << "✅ graph.json updated!\n";

    // =========================
    // 🧪 DEBUG OUTPUT
    // =========================

    cout << "\n=== DEBUG ===\n";
    cout << "Total Files: " << totalFiles << endl;
    cout << "Built Files: " << builtFiles << endl;
    cout << "Optimization: " << optimization << "%" << endl;
    cout << "Time Taken: " << timeTaken << "s\n";

    return 0;
}