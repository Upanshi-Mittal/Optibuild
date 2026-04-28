#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
namespace fs = std::filesystem;
using namespace std;

unordered_map<string, vector<string>> graph;
unordered_map<string, vector<string>> reverseGraph;
unordered_map<string, fs::file_time_type> lastWriteTime;

void loadGraph(const string &filename)
{
    ifstream file(filename);
    string line;

    while (getline(file, line))
    {
        size_t pos = line.find("|");
        string fileNode = fs::weakly_canonical(line.substr(0, pos)).string();
        size_t start = pos;
        while (start != string::npos)
        {
            size_t next = line.find("|", start + 1);
            string dep = fs::weakly_canonical(line.substr(start + 1, next - start - 1)).string();
            if (find(graph[fileNode].begin(), graph[fileNode].end(), dep) == graph[fileNode].end())
            {
                graph[fileNode].push_back(dep);
            }
            reverseGraph[dep].push_back(fileNode);

            start = next;
        }
    }
}

vector<string> findImpacted(const vector<string> &changedFiles)
{
    vector<string> result;
    unordered_map<string, bool> visited;
    queue<string> q;

    for (auto &f : changedFiles)
    {
        q.push(f);
        visited[f] = true;
    }

    while (!q.empty())
    {
        string curr = q.front();
        q.pop();
        result.push_back(curr);

        if (reverseGraph.find(curr) != reverseGraph.end())
        {
            for (auto &dep : reverseGraph[curr])
            {
                if (!visited[dep])
                {
                    visited[dep] = true;
                    q.push(dep);
                }
            }
        }
    }

    return result;
}

vector<string> scanChanges(const string& rootDir) {
    vector<string> changed;

    for (auto& p : fs::recursive_directory_iterator(rootDir)) {
        if (p.path().extension() == ".cpp" || p.path().extension() == ".h") {

            string file = fs::weakly_canonical(p.path()).string();
            auto currentTime = fs::last_write_time(p);

            if (!lastWriteTime.count(file)) {
                lastWriteTime[file] = currentTime;
                continue;
            }

            if (lastWriteTime[file] != currentTime) {
                cout << " Changed: " << file << endl;
                changed.push_back(file);
                lastWriteTime[file] = currentTime;
            }
        }
    }

    return changed;
}

int main() {
    loadGraph("dependency_graph.cache");

    string srcDir = "../src";

    cout << "Watching for file changes...\n";

    while (true) {

        vector<string> changed = scanChanges(srcDir);

        if (!changed.empty()) {

            cout << "\n=== Changed Files ===\n";
            for (auto& f : changed) cout << f << endl;

            auto impacted = findImpacted(changed);

            cout << "\n=== Impacted Files ===\n";
            for (auto& f : impacted) cout << f << endl;

            ofstream out("impacted_files.txt");
            for (auto& f : impacted) out << f << endl;
            out.close();
        }

        this_thread::sleep_for(chrono::seconds(2));
    }

    return 0;
}