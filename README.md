# OptiBuild

OptiBuild is a reusable selective-build optimizer for C++ projects. It scans a project, builds a directed dependency graph from local includes, detects changed files with hashes, finds affected dependents with reverse-graph BFS, and exposes live status for a React dashboard.

It is designed to work with CMake projects, Makefile projects, and simple plain C++ folders.

## Build OptiBuild

```bash
cmake -S . -B build
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Using OptiBuild in Any C++ Project

```bash
# Step 1: Go to any C++ project
cd my-cpp-project

# Step 2: Initialize OptiBuild
optibuild init

# Step 3: Edit .optibuild/config.json if needed
nano .optibuild/config.json

# Step 4: Run first scan
optibuild scan

# Step 5: Start live watcher
optibuild watch

# Step 6: In another terminal, start dashboard
optibuild dashboard
```

From this repo during development, use:

```bash
./build/optibuild init --project /path/to/cpp/project
./build/optibuild scan --project /path/to/cpp/project
./build/optibuild watch --project /path/to/cpp/project
./build/optibuild dashboard --project /path/to/cpp/project
```

Direct combined style:

```bash
./build/optibuild --project /path/to/cpp/project --watch --dashboard
```

Development dashboard mode:

```bash
# Terminal 1
./build/optibuild dashboard --project ../sample_project

# Terminal 2
cd frontend
npm install
npm run dev
```

Open the Vite URL, usually `http://127.0.0.1:5173/`.

## CLI Commands

```bash
optibuild init
optibuild scan
optibuild build
optibuild test
optibuild watch
optibuild dashboard
optibuild status
```

All commands accept:

```bash
--project /path/to/cpp/project
```

## Config File

`optibuild init` creates `.optibuild/config.json`:

```json
{
  "projectName": "MyCppProject",
  "projectRoot": ".",
  "sourceDirs": ["src", "include"],
  "buildDir": "build",
  "buildCommand": "cmake --build build",
  "testCommand": "ctest --test-dir build --output-on-failure",
  "dashboardPort": 5173,
  "apiPort": 8080,
  "watchExtensions": [".cpp", ".hpp", ".h", ".cc", ".cxx"],
  "ignoreDirs": ["build", ".git", "node_modules", ".optibuild"]
}
```

OptiBuild detects CMake and Makefile projects and suggests default build/test commands.

## Live Change Detection and Dashboard Updates

OptiBuild supports live change detection using watch mode. When watch mode is enabled, the tool checks project files at regular intervals. It stores file metadata such as file path, last modified time, and file hash. On every scan cycle, OptiBuild compares the current metadata with the previous cache.

If a file has changed, OptiBuild marks it as a changed file. Then it uses the reverse dependency graph to find all files that depend on the changed file.

```text
1. User modifies a source or header file.
2. File watcher detects the changed timestamp or hash.
3. OptiBuild marks the file as changed.
4. Reverse dependency graph traversal begins.
5. BFS or DFS finds all affected files.
6. Affected files are written to .optibuild/status.json.
7. Backend API exposes the updated status.
8. React dashboard fetches the latest status every second.
9. Dashboard updates changed files, affected files, and metrics live.
```

The dashboard does not need to be refreshed manually. It uses polling to request the latest data from the backend API every second.

Example React polling logic:

```jsx
useEffect(() => {
  const fetchStatus = async () => {
    try {
      const response = await fetch("http://localhost:8080/api/status");
      const data = await response.json();
      setStatus(data);
      setConnected(true);
    } catch (error) {
      setConnected(false);
    }
  };

  fetchStatus();
  const interval = setInterval(fetchStatus, 1000);

  return () => clearInterval(interval);
}, []);
```

This makes OptiBuild useful as a live developer tool. As soon as the developer changes a file, the dashboard shows the changed file, affected files, skipped files, and estimated rebuild reduction.

API endpoints:

```text
GET  /api/status
GET  /api/files
GET  /api/graph
POST /api/scan
POST /api/build
POST /api/test
```

The dashboard shows:

- Live/Disconnected API state
- Changed files
- Affected files
- Rebuild reduction percentage
- Latest events timeline
- Dependency graph
- Scan, Build, and Test buttons

## Selective Build Scope

OptiBuild has two levels:

- Level 1: Always provide dependency analysis, changed files, affected files, and rebuild reduction. The configured build command is run for correctness.
- Level 2: Generate an optimized build plan. Exact per-file compilation depends on build-system internals and is future work for CMake target mapping, Ninja integration, or compile database support.

OptiBuild does not claim perfect universal selective compilation for every C++ build system.

## Complexity

Let `V` be files and `E` be dependencies.

```text
Building dependency graph: O(V + E)
Detecting changed files using hash map: O(V)
Finding affected files using BFS/DFS: O(V + E)
Dependency lookup using unordered_map: Average O(1)
Duplicate prevention using unordered_set: Average O(1)

Overall complexity per scan: O(V + E)
```
