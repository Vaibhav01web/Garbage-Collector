# Memory Compaction & Garbage Collector Engine — Demonstration & Guide

This guide walks you through the entire project: how it works, how each data structure and algorithm operates, and how to trigger and observe the **memory compaction feature** both visually in the browser and via the command line.

---

## 1. Quick Start

### Running the System
The C++ backend server is already built and listens on port `8080`.

To start or rebuild it manually:
```bash
make run
# Or directly:
./gc_engine 8080
```

### Automated Command-Line Walkthrough
To run the complete automated demonstration directly in your terminal with colored visual heap maps:
```bash
python3 demo_walkthrough.py
```

### Accessing the Interactive Visualizer
Open your browser and navigate to:
```
http://localhost:8080
```
*(You can also double-click `frontend/index.html` to open it directly as a local file; it connects automatically to `http://localhost:8080` via CORS).*

---

## 2. Core Architecture & Algorithms

The engine simulates an in-memory runtime heap (similar to the JVM or Google Chrome V8 engine):

```
                        ┌────────────────────────┐
                        │   Stack / GC Roots     │
                        └──────────┬─────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        Heap (512-Byte Array)                           │
│  [Obj 1: 32B] [Obj 2: 24B] [Free: 40B] [Obj 3: 48B] [Free: 368B]      │
└────────────────────────────────────────────────────────────────────────┘
            │                                           ▲
            ▼                                           │
┌────────────────────────┐                  ┌───────────────────────────┐
│ Live Object BST        │                  │ LISP2 Compaction Engine   │
│ (Indexed by offset)    │                  │ (Pointer forwarding &     │
│                        │                  │  sliding live objects)    │
└────────────────────────┘                  └───────────────────────────┘
```

### Core Data Structures
1. **1D Memory Array (`std::vector<uint8_t> heap`)**:
   - Fixed size (default 512 continuous byte slots).
   - Value `0` indicates free space. Value `id` indicates the owning object.
2. **Live Object Binary Search Tree (`ObjectBST`)**:
   - Tracks active objects ordered by their starting memory byte offset.
   - Allows $O(\log N)$ search by offset, fast gap finding, and in-order linear traversals.
3. **Directed Object Reference Graph**:
   - Each object has an ID, type name, size, offset, root flag (`is_root`), and an adjacency list of referenced child IDs (`references`).

### The 3 Garbage Collection Phases
1. **Phase 1: Mark (Stack-based Traversal)**
   - Traverses the heap starting from all objects with `is_root == true`.
   - Uses an explicit `std::stack<int>` (depth-first search) to trace child references.
   - Sets `marked = true` on all reachable objects.
2. **Phase 2: Sweep**
   - Iterates through the heap.
   - Unmarked objects are discarded from the BST and their bytes on the heap are cleared back to `0`.
   - Reclaims freed space, but leaves fragmented holes between surviving live objects.
3. **Phase 3: Compact (LISP2 Compactor Algorithm)**
   - **Pass 1 — Compute Forwarding Offsets**: Scans live objects in increasing memory offset order and calculates their new compacted destination addresses starting from offset `0`.
   - **Pass 2 — Update References**: Updates every internal pointer reference in every surviving object so that references point to the new addresses of their targets.
   - **Pass 3 — Slide Objects**: Relocates live object bytes toward low memory and zeros out the trailing memory, consolidating all free gaps into one single contiguous block.

---

## 3. Step-by-Step Demonstration: Fragmentation Resolution

This demonstration showcases the **unique feature**: resolving allocation failures caused by memory fragmentation.

### The Scenario
Suppose total free memory is **180 bytes**, but an incoming allocation requires **100 contiguous bytes**.
If memory is scattered across multiple 30–40 byte gaps, the allocation **fails** even though total free space is sufficient. Compaction slides surviving blocks together so the allocation succeeds.

### Walkthrough in the Web Interface

1. **Reset to Clean State**:
   - Click the red **Reset Heap** button.
   - Memory Statistics will show `Used Memory: 0 B`, `Live Objects: 0`.

2. **Load the Fragmentation Scenario**:
   - Click the purple **Fragmentation Demo** button.
   - The engine automatically allocates 10 objects labeled `a` through `j`, sets up cross-references, and removes root status from objects `b`, `e`, `h`, `i`, and `j`.
   - Notice the status column in the **Object Table**: objects `b, e, h, i, j` turn into red `GARBAGE` badges.

3. **Observe the Free Gaps (Pre-Compaction)**:
   - Click **2. Sweep** (or observe the log).
   - The dead objects are swept, leaving free gaps scattered between surviving objects.
   - Check **Memory Statistics**:
     - *Total Free Memory*: ~200+ bytes available.
     - *Largest Free Block*: significantly smaller than the total free memory.

4. **Attempt Allocation (Expected: Failure due to Fragmentation)**:
   - In the **Allocate Object** form, enter:
     - **Type**: `Buffer`
     - **Size**: `100`
     - Leave **Auto-GC on failure** unchecked.
   - Click **Allocate**.
   - **Result**: You will receive a red error toast notification:
     ```
     Allocation failed! Total free: 180B, Largest gap: 48B
     ```
   - Total free memory was ample, but no individual contiguous slot was large enough!

5. **Run Compaction**:
   - Click **3. Compact** (or **Full GC**).
   - Watch the animated heap cells slide leftward!
   - All live objects (`a`, `c`, `d`, `f`, `g`) are packed together at low memory offsets `[0 ... 143]`.
   - Notice the **Fragmentation Index** drops to `0%`, and **Largest Free Block** jumps up to a continuous block of **368 bytes**!

6. **Re-attempt Allocation (Expected: Success)**:
   - Click **Allocate** with Size `100` again.
   - **Result**: A green toast notification appears:
     ```
     Allocated Buffer (100B) → id=11
     ```
   - Memory compaction successfully resolved the allocation deadlock!

---

## 4. Running the Demo via cURL / Terminal

You can run this exact demonstration directly from your terminal using the REST API:

```bash
# 1. Reset the heap
curl -s -X POST http://localhost:8080/api/reset

# 2. Setup the fragmentation demo scenario
curl -s -X POST http://localhost:8080/api/demo > /dev/null

# 3. Run Mark and Sweep (leaves holes where garbage objects were removed)
curl -s -X POST http://localhost:8080/api/gc -H 'Content-Type: application/json' -d '{"phase":"mark"}' > /dev/null
curl -s -X POST http://localhost:8080/api/gc -H 'Content-Type: application/json' -d '{"phase":"sweep"}' > /dev/null

# 4. Attempt to allocate a 100-byte buffer (FAILS due to fragmentation)
curl -s -X POST http://localhost:8080/api/allocate \
  -H 'Content-Type: application/json' \
  -d '{"name":"LargeBuffer","size":100,"is_root":true,"with_gc":false}'

# Output:
# {"success":false,"error":"Allocation failed","total_free":304,"largest_free":64}
# -> Fails because largest_free (64B) < 100B, despite 304B total free!

# 5. Run Compaction
curl -s -X POST http://localhost:8080/api/gc -H 'Content-Type: application/json' -d '{"phase":"compact"}' > /dev/null

# 6. Retry the 100-byte allocation (SUCCEEDS!)
curl -s -X POST http://localhost:8080/api/allocate \
  -H 'Content-Type: application/json' \
  -d '{"name":"LargeBuffer","size":100,"is_root":true,"with_gc":false}'

# Output:
# {"success":true,"id":11}
```

---

## 5. Pure C++ Usage Example

If you want to use the engine directly inside a C++ application without the HTTP server, include `src/gc_engine.h`:

```cpp
#include "src/gc_engine.h"
#include <iostream>

int main() {
    // Create an engine with a 512-byte heap
    GCEngine engine(512);

    // 1. Allocate root objects
    int objA = engine.allocate(32, "NodeA", true);  // Root
    int objB = engine.allocate(24, "NodeB", false); // Child (non-root)

    // 2. Link reference: A -> B
    engine.add_reference(objA, objB);

    // 3. Allocate unreferenced garbage
    int junk = engine.allocate(48, "TempJunk", false); // Unreachable

    // 4. Run Mark & Sweep
    engine.mark_phase();
    engine.sweep_phase(); // junk is reclaimed!

    // 5. Run LISP2 Compaction
    engine.compact_phase(); // objA and objB packed together

    // 6. Query statistics
    std::cout << "Live objects: " << engine.get_live_object_count() << "\n";
    std::cout << "Used bytes:   " << engine.get_used_bytes() << "\n";
    std::cout << "Largest gap:  " << engine.get_largest_free_block() << " bytes\n";

    return 0;
}
```

---

## 6. REST API Reference

| Endpoint | Method | Payload / Parameters | Description |
|---|---|---|---|
| `/` or `/index.html` | `GET` | None | Serves the interactive visualizer HTML |
| `/api/state` | `GET` | None | Returns complete heap array, BST nodes, objects, references, and logs |
| `/api/health` | `GET` | None | Health check (`{"status":"ok"}`) |
| `/api/allocate` | `POST` | `{"name":"String", "size":24, "is_root":true, "with_gc":false}` | Allocates an object on the heap |
| `/api/reference` | `POST` | `{"from":1, "to":2}` | Adds a directed reference edge from object 1 to object 2 |
| `/api/remove-root` | `POST` | `{"id":1}` | Removes root status, making the object eligible for collection if unreferenced |
| `/api/gc` | `POST` | `{"phase":"full"}` / `"mark"` / `"sweep"` / `"compact"` | Triggers full GC or a single individual phase |
| `/api/demo` | `POST` | None | Resets heap and sets up the fragmented memory scenario |
| `/api/random` | `POST` | `{"count":5}` | Allocates random objects of varying sizes |
| `/api/reset` | `POST` | None | Wipes all objects and resets the heap to 0 |
