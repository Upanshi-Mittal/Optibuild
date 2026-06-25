# OptiBuild

OptiBuild is a reusable C++ project scanner that helps developers understand which source files are affected by a change. It builds a directed include graph, stores file hashes in a cache, runs reverse-graph BFS to find impacted files, and writes JSON that a React dashboard or another tool can consume.

You can run a single scan like `git status`, or use watch mode to keep the JSON/dashboard updated while you edit files.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Use With Any C++ Project

The default project path is the current directory:

```bash
cd /path/to/cpp/project
optibuild --scan
```

From this repository during development:

```bash
./build/optibuild --project /path/to/cpp/project --scan
```

OptiBuild creates a per-project `.optibuild` directory:

```text
.optibuild/
  config.json   # project settings
  cache.json    # file hashes and dependency cache
  status.json   # latest scan result for dashboards/tools
  output.json   # dashboard summary JSON
  graph.json    # dependency graph JSON
```

## CLI Options

```bash
optibuild --scan
optibuild --watch
optibuild --build
optibuild --test
optibuild --project /path/to/cpp/project --scan
optibuild --project /path/to/cpp/project --watch --output /path/to/report-dir
optibuild --project /path/to/cpp/project --scan --output /path/to/report-dir
```

Equivalent command style is also supported:

```bash
optibuild scan --project /path/to/cpp/project
optibuild watch --project /path/to/cpp/project
optibuild build --project /path/to/cpp/project
optibuild test --project /path/to/cpp/project
optibuild status --project /path/to/cpp/project
```

`--output <path>` controls where `output.json` and `graph.json` are written. The config, cache, and status files always stay inside the target project’s `.optibuild` directory.

To preview the bundled React dashboard during development, write the report into Vite's public directory:

```bash
./build/optibuild --project /path/to/cpp/project --scan --output frontend/public
cd frontend
npm install
npm run dev
```

For live dashboard updates, keep watch mode running in one terminal:

```bash
./build/optibuild --project /path/to/cpp/project --watch --output frontend/public
```

Then run the dashboard in another terminal:

```bash
cd frontend
npm run dev
```

Watch mode treats `.optibuild/cache.json` like a baseline. It keeps showing changed files until you run a normal scan again:

```bash
./build/optibuild --project /path/to/cpp/project --scan
```

## Config

OptiBuild auto-creates `.optibuild/config.json` on the first scan:

```json
{
  "projectName": "MyCppProject",
  "projectRoot": "/path/to/cpp/project",
  "sourceDirs": ["src", "include"],
  "buildDir": "build",
  "buildCommand": "cmake --build build",
  "testCommand": "ctest --test-dir build --output-on-failure",
  "fileExtensions": [".cpp", ".hpp", ".h", ".cc", ".cxx"],
  "ignoreDirs": ["build", ".git", "node_modules", ".optibuild"]
}
```

For Makefile projects, OptiBuild detects `make` and `make test`. For simple folders, it creates placeholder commands that you can edit.

## How It Works

1. Scan configured source directories.
2. Hash C++ source/header files.
3. Parse local includes such as `#include "auth.h"`.
4. Build a directed graph where `A -> B` means `A` includes `B`.
5. Build the reverse graph where `B -> A` means `A` depends on `B`.
6. Compare current hashes with `.optibuild/cache.json`.
7. Run BFS from changed files through the reverse graph.
8. Write `.optibuild/status.json`, `.optibuild/cache.json`, `output.json`, and `graph.json`.

## JSON Outputs

`.optibuild/status.json` is the main frontend-compatible result:

```json
{
  "schemaVersion": 2,
  "projectName": "my-project",
  "projectRoot": "/path/to/my-project",
  "lastScanTime": "2026-06-25 12:37:03",
  "watchMode": false,
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

`output.json` provides summary data for dashboard cards. `graph.json` contains graph nodes and directed edges for visualization.

## Git-Style Status

After a scan or while watch mode is running, use:

```bash
optibuild status --project /path/to/cpp/project
```

This prints changed files, affected files, and rebuild candidates in a readable summary.

## Build and Test Commands

`--build` runs a scan first, then runs the configured build command from the project root:

```bash
optibuild --project /path/to/cpp/project --build
```

`--test` runs a scan first, then runs the configured test command:

```bash
optibuild --project /path/to/cpp/project --test
```

## Complexity

Let `V` be files and `E` be include dependencies.

```text
Graph scan: O(V + E)
Hash comparison with unordered_map: O(V)
Reverse dependency BFS: O(V + E)
Duplicate prevention with unordered_set: O(1) average per insert
```

## Current Limitations

OptiBuild resolves only local quote includes such as:

```cpp
#include "auth.h"
```

It does not fully resolve compiler include paths, generated files, macros, third-party headers, or exact build-system targets yet. The current build mode runs the configured project build command for correctness after producing the affected-file plan.

## TODO: Future Live Mode

- Replace polling watch mode with native filesystem events where available.
- Add a small local API server for dashboard actions.
- Let the React dashboard poll `.optibuild/status.json` through the API.
- Add historical scan metrics.
- Integrate `compile_commands.json` for compiler-accurate include resolution.
- Add Ninja/CMake target mapping for more exact selective builds.
