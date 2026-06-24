# OptiBuild — Smart Project Optimization Platform

OptiBuild is a C++-based project optimization system designed to improve workflow efficiency and resource allocation in complex task environments. It analyzes task dependencies, identifies bottlenecks, and provides data-driven insights to enhance productivity.
The project simulates real-world project management scenarios where multiple tasks, constraints, and resources must be balanced efficiently. It focuses on logical problem-solving, algorithmic optimization, and performance-oriented design.

---

## Key Features

- Workflow Analysis — Evaluates task flow and execution paths  
- Resource Allocation — Optimizes distribution of available resources  
- Bottleneck Detection — Identifies inefficiencies in project pipelines  
- Performance Optimization — Improves execution efficiency using data insights  
- Task Dependency Handling — Models relationships between tasks  

---

## Tech Stack

- **Language:** C++  
- **Concepts Used:**  
  - Data Structures (Vectors, Maps, Graphs)  
  - Algorithms (Traversal, Optimization Logic)  
- **Tools:** STL, File Handling  

---

## System Overview

The system models a project as a structured set of tasks with dependencies. It processes input data, builds relationships between tasks, and applies logical analysis to:

- Determine execution order  
- Detect delays and inefficiencies  
- Suggest optimized workflow paths  

---

## How It Works

1. Input project/task data  
2. Build dependency graph  
3. Analyze execution flow  
4. Detect bottlenecks  
5. Optimize task scheduling and resource usage  

---

## Use Cases

- Project Management Simulation  
- Workflow Optimization Systems  
- Resource Planning Tools  
- Algorithmic Problem Solving Practice  

---

## Future Improvements

- Add visualization for workflow graphs  
- Integrate a web-based frontend  
- Implement real-time data processing  
- Extend to multi-user project environments  
=======
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

Watch mode uses portable polling:

```text
1. Load config.
2. Load previous cache.
3. Scan watched files.
4. Compare hashes.
5. Rebuild the dependency graph.
6. Traverse reverse dependencies with BFS.
7. Write .optibuild/status.json.
8. Dashboard polls /api/status every 1 second.
```

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
