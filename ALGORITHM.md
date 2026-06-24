# OptiBuild Algorithm

## Goal

Given a configured C++ project, find the minimum useful rebuild set after file changes. If a header changes, all files that directly or indirectly depend on that header should be marked affected.

## Steps

```text
1. Load .optibuild/config.json.
2. Scan configured source directories.
3. Collect watched files such as .cpp, .cc, .cxx, .h, and .hpp.
4. Parse local #include "..." statements.
5. Build a directed dependency graph.
6. Load previous hashes from .optibuild/cache.json.
7. Compare current hashes with cached hashes.
8. Mark changed files.
9. Traverse the reverse graph with BFS.
10. Mark dependent files as affected.
11. Generate an optimized rebuild plan.
12. Export .optibuild/status.json for the API/dashboard.
13. Save the updated cache.
```

## Graph Model

An edge `A -> B` means:

```text
File A includes or depends on file B.
```

For impact analysis, OptiBuild uses the reverse graph:

```text
If B changes, find every A that depends on B.
```

## BFS Impact Traversal

Input:

- Changed files detected from hash comparison.

Output:

- Changed files plus all files that depend on them directly or indirectly.

Process:

```text
queue = changed files
affected = changed files

while queue is not empty:
    current = queue.pop()
    for each dependent in reverseGraph[current]:
        if dependent has not been visited:
            affected.add(dependent)
            queue.push(dependent)
```

This catches chains such as:

```text
main.cpp -> auth.h -> database.h
```

If `database.h` changes, `auth.h` and `main.cpp` can be marked affected through reverse traversal.

## Selective Build Decision

OptiBuild rebuilds compilable source files that are marked affected:

- `.cpp`
- `.cc`
- `.cxx`

Headers are tracked as changed or affected, but they are not compiled directly. Their dependent source files become rebuild candidates.

## Live Change Detection and Dashboard Updates

Watch mode repeats the same scan algorithm every 1 second. When content hashes change, status is rewritten:

```text
watch loop:
    scan project
    update cache
    update status.json
    sleep 1 second
```

The dashboard does not read source files directly. It polls the API:

```text
GET /api/status -> .optibuild/status.json
```

This keeps the system simple and reliable for an MVP.

## Complexity

Let:

- `V` = number of files
- `E` = number of dependency edges

Complexity:

```text
Building dependency graph: O(V + E)
Detecting changed files using hash map: O(V)
Finding affected files using BFS: O(V + E)
Dependency lookup using unordered_map: Average O(1)
Duplicate prevention using unordered_set: Average O(1)

Overall complexity per scan: O(V + E)
```

## Why Hashes Instead of Only Timestamps

Timestamps can change even when file content does not, and they can also be unreliable across copies or tooling. Hash-based detection compares actual content, which makes OptiBuild's changed-file detection more accurate.
