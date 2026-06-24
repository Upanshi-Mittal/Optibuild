# OptiBuild Project Report

## Project Summary

OptiBuild is a selective build optimization tool for C++ projects. It demonstrates how dependency analysis, hashing, graph traversal, and caching can reduce unnecessary rebuild work.

The project includes:

- A C++ analyzer backend.
- A reusable CLI for any C++ project path.
- Per-project `.optibuild/config.json`, cache, and status files.
- A polling API server for live dashboard data.
- A React dashboard frontend.
- Cache-based change detection.
- BFS-based impact analysis.
- JSON exports for visualization.

## Problem Statement

C++ builds can become slow because one small change often triggers broad rebuilds. This is especially inefficient in academic, CI, and local development workflows where developers frequently modify a small number of files.

OptiBuild addresses this by identifying only the files affected by a change and producing a focused rebuild plan.

## Implemented Solution

The analyzer scans source files and parses local include statements. It builds a directed dependency graph using an adjacency list:

```text
A -> B means A depends on B
```

It also builds a reverse dependency graph:

```text
B -> A means A depends on B
```

The reverse graph allows fast impact analysis when a dependency changes.

## Core Features

- Directed graph representation for dependencies.
- Sparse adjacency-list storage.
- Reverse dependency tracking.
- Hash-based file-change detection.
- `.optibuild/cache.json` cache file.
- BFS traversal for affected files.
- Selective rebuild plan for affected source files.
- Dashboard JSON export.
- React dashboard with metrics, file table, and graph visualization.
- CLI modes for scan, build, test, dashboard data, once, and watch.
- Generic commands: `init`, `scan`, `build`, `test`, `watch`, `dashboard`, and `status`.
- API endpoints for status, files, graph, scan, build, and test.

## Using OptiBuild in Any C++ Project

The intended user workflow is:

```bash
cd my-cpp-project
optibuild init
optibuild scan
optibuild watch
```

In another terminal:

```bash
optibuild dashboard
```

The user can then edit a file such as `auth.h`. Within the next polling interval, OptiBuild detects the change, recalculates affected files, writes status, and the dashboard updates.

## Live Change Detection and Dashboard Updates

The watcher uses `std::filesystem` polling every 1 second for portability. The API server exposes:

```text
GET  /api/status
GET  /api/files
GET  /api/graph
POST /api/scan
POST /api/build
POST /api/test
```

The React dashboard polls `/api/status` every second. It shows a green Live indicator when connected and a Disconnected state when the API is unreachable.

## Data Structures Used

```cpp
struct FileInfo {
    std::string path;
    std::string hash;
    std::time_t lastModified;
    std::size_t dependencyCount;
    bool changed;
    bool affected;
};
```

Primary containers:

- `unordered_map<string, vector<string>>` for adjacency list.
- `unordered_map<string, vector<string>>` for reverse graph.
- `unordered_map<string, FileInfo>` for metadata.
- `unordered_set<string>` for visited files and duplicate prevention.
- `queue<string>` for BFS traversal.

## Output

The analyzer generates:

- `frontend/public/output.json`
- `frontend/public/graph.json`
- `.optibuild/cache.json`

These files allow the dashboard to display:

- Total files scanned
- Changed files
- Affected files
- Skipped files
- Estimated rebuild reduction percentage
- Dependency graph summary
- Last scan time

## Limitations

OptiBuild currently parses local quote includes such as:

```cpp
#include "auth.h"
```

It does not fully resolve compiler include paths, generated files, macros, or third-party library headers. Those would be good future enhancements.

For real C++ projects, exact selective compilation depends on build-system internals. The current implementation always provides dependency analysis and then runs the configured build command for correctness. More exact per-target compilation can be added later through `compile_commands.json`, Ninja metadata, or CMake target mapping.

## Future Enhancements

- Parse compile commands from `compile_commands.json`.
- Integrate directly with Ninja or Make for exact object rebuild commands.
- Add richer historical metrics.
- Add test selection based on source ownership.
- Support multiple source roots.
- Add CI mode that fails when stale cache data is detected.

## Conclusion

OptiBuild demonstrates a practical selective build workflow using efficient data structures and graph algorithms. The system remains simple enough for an academic software engineering project while showing professional architecture, measurable complexity, and a polished dashboard.
