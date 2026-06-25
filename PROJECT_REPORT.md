# OptiBuild Project Report

## Project Summary

OptiBuild is a reusable selective-build analysis tool for C++ projects. It scans a project once, builds an include dependency graph, detects changed files using hashes, finds affected dependents, and writes frontend-friendly JSON files.

The goal is to make C++ build impact analysis understandable for students while keeping the implementation close to a real developer tool.

## Current MVP Scope

Implemented:

- Generic project path support with `--project <path>`.
- Current directory as the default project path.
- Clear commands: `--scan`, `--watch`, `--build`, and `--test`.
- Optional `--output <path>` for dashboard JSON artifacts.
- Per-project `.optibuild/config.json`.
- Per-project `.optibuild/cache.json`.
- Per-project `.optibuild/status.json`.
- Directed dependency graph from local includes.
- Reverse dependency graph for impact analysis.
- Hash-based change detection.
- BFS-based affected-file discovery.
- JSON output for a React dashboard.
- Polling-based watch mode for live updates while editing.
- Git-style `status` output for changed, affected, and rebuild files.

Not implemented yet:

- Local dashboard API server.
- Exact per-target build-system integration.

## CLI Design

Main usage:

```bash
./optibuild --project /path/to/cpp/project --scan
./optibuild --project /path/to/cpp/project --watch --output frontend/public
./optibuild --project /path/to/cpp/project --build
./optibuild --project /path/to/cpp/project --test
./optibuild --project /path/to/cpp/project --scan --output /tmp/optibuild-report
```

If `--project` is omitted, OptiBuild scans the current directory.

The analyzer runs once by default. Watch mode is opt-in, which keeps scripts and CI predictable while still supporting live local development.

## Generated Files

OptiBuild writes project-owned state into `.optibuild`:

```text
.optibuild/
  config.json
  cache.json
  status.json
  output.json
  graph.json
```

`config.json` stores project settings:

```json
{
  "projectName": "my-project",
  "projectRoot": "/path/to/my-project",
  "sourceDirs": ["src", "include"],
  "buildDir": "build",
  "buildCommand": "cmake --build build",
  "testCommand": "ctest --test-dir build --output-on-failure",
  "fileExtensions": [".cpp", ".hpp", ".h", ".cc", ".cxx"],
  "ignoreDirs": ["build", ".git", "node_modules", ".optibuild"]
}
```

`cache.json` stores hashes and dependency edges:

```json
{
  "version": 1,
  "generatedAt": "2026-06-25 12:37:03",
  "files": [
    {
      "path": "/path/to/src/main.cpp",
      "hash": "692292d4f68701cb",
      "lastModified": 1782362223,
      "dependencyCount": 2
    }
  ],
  "dependencies": [
    {
      "from": "/path/to/src/main.cpp",
      "to": "/path/to/src/auth/auth.h"
    }
  ]
}
```

`status.json` stores the latest scan result:

```json
{
  "schemaVersion": 2,
  "projectName": "my-project",
  "projectRoot": "/path/to/my-project",
  "watchMode": true,
  "totalFiles": 9,
  "sourceFiles": 5,
  "dependencyEdges": 10,
  "changedFiles": [],
  "affectedFiles": [],
  "rebuildFiles": [],
  "skippedFiles": 5,
  "rebuildReductionPercent": 100.0,
  "files": [],
  "graph": { "edges": [] },
  "events": []
}
```

## Data Structure Explanation

### File Metadata

```cpp
struct FileInfo {
    std::string path;
    std::string hash;
    std::time_t lastModified = 0;
    std::size_t dependencyCount = 0;
    bool changed = false;
    bool affected = false;
};
```

Each scanned file has its path, content hash, timestamp, dependency count, and scan status.

### Directed Dependency Graph

```cpp
std::unordered_map<std::string, std::unordered_set<std::string>> adjacencyList;
```

Meaning:

```text
A -> B means file A includes file B.
```

Example:

```text
main.cpp -> auth.h
main.cpp -> billing.h
```

`unordered_map` gives average O(1) lookup for a file. `unordered_set` prevents duplicate include edges efficiently.

### Reverse Dependency Graph

```cpp
std::unordered_map<std::string, std::unordered_set<std::string>> reverseGraph;
```

Meaning:

```text
B -> A means file A depends on file B.
```

This graph is the key to impact analysis. If `auth.h` changes, OptiBuild asks: which files depend on `auth.h`?

### Hash Cache

```cpp
std::unordered_map<std::string, std::string> cachedHashes;
```

The cache maps file path to the previous scan hash. During the next scan, OptiBuild compares the new hash with the cached hash.

### BFS Queue

```cpp
std::queue<std::string> queue;
std::unordered_set<std::string> affected;
```

The BFS starts from changed files and walks the reverse dependency graph:

```text
changed header -> direct dependents -> indirect dependents -> rebuild candidates
```

This avoids scanning unrelated branches of the graph.

## Algorithm

```text
1. Resolve the project root from --project or current directory.
2. Load or create .optibuild/config.json.
3. Load .optibuild/cache.json.
4. Scan configured source directories.
5. Hash files with supported extensions.
6. Parse local quote includes.
7. Build adjacencyList and reverseGraph.
8. Compare current hashes with cached hashes.
9. Push changed files into a BFS queue.
10. Traverse reverseGraph to find affected files.
11. Filter affected compilable files into rebuildFiles.
12. Write status, cache, output, and graph JSON.
```

## Complexity

Let:

- `V` = scanned files
- `E` = include dependency edges

```text
Scanning files: O(V)
Building graph: O(V + E)
Hash lookup with unordered_map: O(V)
Affected-file BFS: O(V + E)
Duplicate edge prevention with unordered_set: O(1) average
```

Overall scan complexity is O(V + E).

## Frontend JSON Contract

The React dashboard can use:

- `.optibuild/status.json` for current state.
- `.optibuild/output.json` for summary cards and file tables.
- `.optibuild/graph.json` for visual graph rendering.

Important fields for dashboard UI:

- `projectName`
- `projectRoot`
- `lastScanTime`
- `changedFiles`
- `affectedFiles`
- `rebuildFiles`
- `skippedFiles`
- `rebuildReductionPercent`
- `files[].path`
- `files[].status`
- `files[].dependencyCount`
- `graph.edges[]`

## Limitations

OptiBuild currently parses local includes only:

```cpp
#include "auth.h"
```

It does not yet fully model:

- compiler include directories
- generated headers
- macro-based includes
- third-party dependency graphs
- CMake target ownership
- Ninja object-level rebuild plans

For safety, `--build` still runs the configured build command after analysis.

## Watch Mode

Watch mode repeatedly runs the same scan pipeline and rewrites `.optibuild/status.json`, `output.json`, and `graph.json`.

```bash
./optibuild --project /path/to/cpp/project --watch --output frontend/public
```

This lets the dashboard refresh as files change. A comment added to `auth.h`, for example, changes the file hash, marks `auth.h` as changed, and uses the reverse graph to find affected `.cpp` files.

During watch mode, `.optibuild/cache.json` acts like a git index. Watch scans compare against the existing cache but do not overwrite it, so changed files remain visible until the user runs a normal `--scan` to accept the current state as the new baseline.

## TODO: Future Live Improvements

Planned additions:

- Local API server for dashboard actions.
- Native filesystem events where available.
- Configurable polling interval.
- Historical scan timeline.
- More precise target mapping through `compile_commands.json` or Ninja metadata.

## Conclusion

OptiBuild now behaves like a reusable developer tool instead of a hardcoded project demo. It accepts any C++ project path, keeps project-specific state in `.optibuild`, uses efficient graph data structures, and produces stable JSON for a future dashboard workflow.
