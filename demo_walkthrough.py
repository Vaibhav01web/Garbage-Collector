#!/usr/bin/env python3
"""
Interactive / Automated Demonstration Script for Memory Compaction & GC Engine
Runs through the memory fragmentation & compaction scenario step-by-step with live visualizations.
"""

import sys
import json
import time
import urllib.request
import urllib.error

BASE = "http://localhost:8080"

# ANSI Terminal Colors
BOLD = "\033[1m"
GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
MAGENTA = "\033[35m"
BLUE = "\033[34m"
RESET = "\033[0m"

def api(path, method="GET", body=None):
    url = BASE + path
    headers = {"Content-Type": "application/json"}
    data = json.dumps(body).encode("utf-8") if body else None
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.URLError as e:
        print(f"{RED}Error connecting to backend ({url}): {e}{RESET}")
        print("Please ensure './gc_engine 8080' is running.")
        sys.exit(1)

def print_heap_map(cells, width=64):
    print(f"\n{BOLD}Heap Visual Representation (512 Bytes / 8 Rows × 64 Columns):{RESET}")
    print("┌" + "─" * width + "┐")
    for r in range(0, len(cells), width):
        row = cells[r : r + width]
        line = "│"
        for val in row:
            if val == 0:
                line += f"{RESET}·"
            else:
                # Color code by object ID
                colors = [CYAN, GREEN, YELLOW, MAGENTA, BLUE, RED]
                col = colors[val % len(colors)]
                line += f"{col}█"
        line += f"{RESET}│"
        print(line)
    print("└" + "─" * width + "┘")
    print(f"{RESET}Legend: {RESET}· = Free Byte  |  Colored █ = Allocated Object Byte\n")

def print_stats(state):
    h = state["heap"]
    s = state["stats"]
    print(f"{BOLD}Heap Stats:{RESET} Used: {h['used']}/{h['size']} B | "
          f"Total Free: {h['size'] - h['used']} B | "
          f"{YELLOW}Largest Contiguous Gap: {h['largest_free']} B{RESET} | "
          f"Fragmentation: {h['fragmentation']*100:.1f}%")

def banner(title):
    print("\n" + "=" * 70)
    print(f" {BOLD}{CYAN}{title}{RESET}")
    print("=" * 70)

def main():
    banner("1. Checking Engine Connection")
    health = api("/api/health")
    print(f"{GREEN}✓ Connected to C++ Backend Engine:{RESET} {health}")

    banner("2. Resetting Heap to Empty State")
    state = api("/api/reset", method="POST")
    print_heap_map(state["heap"]["cells"])
    print_stats(state)

    banner("3. Loading Memory Fragmentation Scenario (/api/demo)")
    print("Allocating 10 objects (a-j), linking cross-references, and unrooting b, e, h, i, j...")
    state = api("/api/demo", method="POST")
    print_heap_map(state["heap"]["cells"])
    print(f"Live/Active Objects: {len(state['objects'])}")
    print_stats(state)

    banner("4. Running Mark & Sweep Garbage Collection (Leaving Gaps)")
    print("Marking live reachable objects from roots...")
    api("/api/gc", method="POST", body={"phase": "mark"})
    print("Sweeping unreachable garbage objects (b, e, h, i, j)...")
    state = api("/api/gc", method="POST", body={"phase": "sweep"})
    print_heap_map(state["heap"]["cells"])
    print_stats(state)
    total_free = state["heap"]["size"] - state["heap"]["used"]
    largest_gap = state["heap"]["largest_free"]

    banner("5. The Fragmentation Dilemma: Allocation Attempt (100 Bytes)")
    print(f"Incoming request: Allocate a buffer of {BOLD}100 bytes{RESET}.")
    print(f"Analysis:")
    print(f"  • Total Free Space in Heap: {GREEN}{total_free} Bytes{RESET}  (More than enough!)")
    print(f"  • Largest Contiguous Block: {RED}{largest_gap} Bytes{RESET}   (Too small!)")
    print(f"\nAttempting allocation without auto-GC...")

    res = api("/api/allocate", method="POST", body={"name": "Buffer", "size": 100, "is_root": True, "with_gc": False})
    if not res.get("success"):
        print(f"\n{RED}❌ ALLOCATION FAILED AS EXPECTED!{RESET}")
        print(f"Engine response: {res}")
        print(f"{YELLOW}Reason: Memory is too fragmented. No single free hole can fit 100 contiguous bytes.{RESET}")
    else:
        print(f"{GREEN}Unexpected success: {res}{RESET}")

    banner("6. Running LISP2 Memory Compaction (/api/gc compact)")
    print("Executing Two-Finger LISP2 Memory Compaction algorithm:")
    print("  1. Compute forwarding addresses for all surviving live objects.")
    print("  2. Relocate and update all object references across memory.")
    print("  3. Slide live objects toward low memory, fusing all free gaps together.")
    state = api("/api/gc", method="POST", body={"phase": "compact"})
    print_heap_map(state["heap"]["cells"])
    print_stats(state)

    banner("7. Retrying the 100-Byte Allocation Post-Compaction")
    print(f"New Largest Contiguous Block: {GREEN}{state['heap']['largest_free']} Bytes{RESET}!")
    print("Attempting to allocate the same 100-byte Buffer again...")
    res = api("/api/allocate", method="POST", body={"name": "Buffer", "size": 100, "is_root": True, "with_gc": False})

    if res.get("success"):
        print(f"\n{GREEN}✅ ALLOCATION SUCCEEDED! Object ID = {res['id']}{RESET}")
        state = api("/api/state")
        print_heap_map(state["heap"]["cells"])
        print_stats(state)
        print(f"\n{BOLD}{GREEN}Compaction successfully eliminated fragmentation and resolved the memory deadlock!{RESET}\n")
    else:
        print(f"{RED}Allocation failed: {res}{RESET}")

if __name__ == "__main__":
    main()
