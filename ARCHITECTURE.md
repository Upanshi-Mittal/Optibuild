# OptiBuild Architecture

## Overview

OptiBuild has two parts:

- A C++ analyzer that scans source files, creates a dependency graph, detects changes, and exports build-impact data.
- A small C++ HTTP API server that exposes live status from `.optibuild/status.json`.
- A React dashboard that polls the API and visualizes build metrics, file statuses, events, and dependency edges.

## Reusable Project Model

OptiBuild is configured per project with:

```text
.optibuild/config.json
.optibuild/cache.json
.optibuild/status.json
```

The CLI defaults to the current directory, or accepts:

```bash
optibuild --project /path/to/cpp/project
```

## Backend Components

### `DependencyGraph`

Located in `analyzer/dependency_graph.h` and `analyzer/dependency_graph.cpp`.

Responsibilities:

- Store each source/header file as a graph node.
- Store directed edges where `A -> B` means file `A` depends on file `B`.
- Maintain `adjacencyList` for direct dependencies.
- Maintain `reverseGraph` for fast dependent lookup.
- Store `FileInfo` metadata in an `unordered_map`.
- Detect changed files by comparing current hashes with cached hashes.
- Traverse reverse dependencies with BFS to find affected files.
- Export `output.json`, `graph.json`, and `.optibuild/cache.json`.
- Export `.optibuild/status.json` for the live API and dashboard.

### `OptiBuildConfig`

Located in `analyzer/optibuild_config.h` and `analyzer/optibuild_config.cpp`.

Responsibilities:

- Load and save `.optibuild/config.json`.
- Detect default project name, source directories, and build/test commands.
- Support CMake, Makefile, and plain C++ project defaults.

### `optibuild_cli`

Located in `analyzer/optibuild_cli.h` and `analyzer/optibuild_cli.cpp`.

Responsibilities:

- Parse command-line options.
- Run `init`, `scan`, `build`, `test`, `watch`, `dashboard`, and `status`.
- Keep default execution finite unless `watch` is passed.
- Coordinate scan, cache loading, change detection, impact analysis, export, and cache saving.
- Run the simple polling API server used by the dashboard.

## API Server

The `dashboard` command starts a lightweight HTTP server on `apiPort`, default `8080`.

Endpoints:

```text
GET  /api/status
GET  /api/files
GET  /api/graph
POST /api/scan
POST /api/build
POST /api/test
```

The API reads `.optibuild/status.json` for status responses. The watcher updates this file every time a scan completes.

### Compatibility Executables

The project still builds:

- `graph_builder`
- `impact_analyzer`
- `build_optimizer`

They now call the same shared analyzer engine instead of duplicating graph logic.

## Data Structures

```cpp
std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
std::unordered_map<std::string, std::vector<std::string>> reverseGraph;
std::unordered_map<std::string, FileInfo> fileMetadata;
std::unordered_map<std::string, std::string> cachedHashes;
```

Why these structures:

- The dependency graph is sparse, so adjacency lists are more memory-efficient than matrices.
- `unordered_map` gives average `O(1)` lookup for file metadata and dependency lists.
- `unordered_set` is used during traversal and include parsing to avoid duplicates.
- `queue` is used for BFS impact traversal.

## Frontend Components

### `Dashboard.jsx`

Reads `output.json` and `graph.json`, then displays:

- Total files scanned
- Changed files
- Affected files
- Skipped files
- Rebuild reduction percentage
- Scan time
- Build plan
- File impact table

### `Graph.jsx`

Uses React Flow to display directed dependency relationships. Node borders show status:

- Red: changed
- Amber: affected
- Blue/green context: skipped or unaffected

## Data Flow

```text
Any C++ project
  -> .optibuild/config.json
  -> C++ analyzer scan
  -> Dependency graph
  -> Cache comparison
  -> Changed file list
  -> Reverse BFS impact analysis
  -> .optibuild/status.json
  -> HTTP API
  -> React dashboard polling
```
