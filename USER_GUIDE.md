# OptiBuild User Guide

## Install or Build

Build the tool:

```bash
cmake -S . -B build
cmake --build build
```

During development, run it as:

```bash
./build/optibuild
```

After installing or copying it into your `PATH`, run it as:

```bash
optibuild
```

## Start a New Project

```bash
cd my-cpp-project
optibuild init
```

This creates:

```text
.optibuild/config.json
.optibuild/cache.json
.optibuild/status.json
```

Review `.optibuild/config.json` and adjust `sourceDirs`, `buildCommand`, or `testCommand` if needed.

## Run a Scan

```bash
optibuild scan
```

The scan:

- Finds C++ files.
- Parses local `#include "..."` dependencies.
- Compares hashes with `.optibuild/cache.json`.
- Finds changed and affected files.
- Writes `.optibuild/status.json`.

## Watch for Live Changes

```bash
optibuild watch
```

The watcher rescans every 1 second. When you edit a watched file, OptiBuild updates status with changed files, affected files, rebuild files, and event messages.

Live updates work like this:

```text
1. You modify a source or header file.
2. Watch mode detects a changed timestamp or hash.
3. OptiBuild marks the file as changed.
4. Reverse dependency traversal begins.
5. BFS finds files affected by the changed dependency.
6. .optibuild/status.json is updated.
7. The API exposes the latest status.
8. The React dashboard polls every second.
9. Dashboard metrics update without a manual refresh.
```

## Start the Dashboard

```bash
optibuild dashboard
```

In development:

```bash
cd frontend
npm install
npm run dev
```

The dashboard polls:

```text
http://localhost:8080/api/status
```

When connected, the dashboard shows changed files, affected files, skipped files, rebuild reduction, and recent event messages live.

## Build and Test

```bash
optibuild build
optibuild test
```

These commands run dependency analysis first, then execute the configured build or test command from `.optibuild/config.json`.

## Use a Different Project Path

```bash
./build/optibuild init --project ../sample_project
./build/optibuild scan --project ../sample_project
./build/optibuild watch --project ../sample_project
./build/optibuild dashboard --project ../sample_project
```

## View Latest Status

```bash
optibuild status
```

This prints `.optibuild/status.json`.
