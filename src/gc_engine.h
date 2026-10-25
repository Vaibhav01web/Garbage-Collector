

#pragma once

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

static const int HEAP_SIZE = 512;

struct HeapObject {
  int id;                      // Unique object identifier
  std::string type_name;       // Type label (e.g. "String", "HashMap")
  size_t offset;               // Starting byte offset in heap
  size_t size;                 // Size in bytes
  bool marked;                 // GC mark bit
  bool is_root;                // True if reachable from root set
  size_t forward_addr;         // Forwarding address (LISP2 compaction)
  std::vector<int> references; // Outgoing references to other object IDs

  HeapObject()
      : id(-1), offset(0), size(0), marked(false), is_root(false),
        forward_addr(SIZE_MAX) {}

  HeapObject(int id, const std::string &name, size_t offset, size_t size,
             bool root)
      : id(id), type_name(name), offset(offset), size(size), marked(false),
        is_root(root), forward_addr(SIZE_MAX) {}
};

struct BSTNode {
  int object_id;
  size_t key; // Heap offset (sort key)
  BSTNode *left;
  BSTNode *right;

  BSTNode(int id, size_t key)
      : object_id(id), key(key), left(nullptr), right(nullptr) {}
};

class ObjectBST {
public:
  BSTNode *root;

  ObjectBST() : root(nullptr) {}

  ~ObjectBST() { clear(); }

  void insert(int object_id, size_t offset) {
    root = insert_rec(root, object_id, offset);
  }

  void remove(size_t offset) { root = remove_rec(root, offset); }

  BSTNode *find(size_t offset) const { return find_rec(root, offset); }

  void clear() {
    clear_rec(root);
    root = nullptr;
  }

  // In-order traversal → object IDs sorted by ascending offset
  std::vector<int> inorder() const {
    std::vector<int> result;
    inorder_rec(root, result);
    return result;
  }

  // Serialize BST structure to JSON for frontend visualization
  std::string to_json() const { return node_to_json(root); }

private:
  BSTNode *insert_rec(BSTNode *node, int obj_id, size_t offset) {
    if (!node)
      return new BSTNode(obj_id, offset);
    if (offset < node->key)
      node->left = insert_rec(node->left, obj_id, offset);
    else if (offset > node->key)
      node->right = insert_rec(node->right, obj_id, offset);
    return node;
  }

  BSTNode *find_min(BSTNode *node) const {
    while (node && node->left)
      node = node->left;
    return node;
  }

  BSTNode *remove_rec(BSTNode *node, size_t offset) {
    if (!node)
      return nullptr;
    if (offset < node->key) {
      node->left = remove_rec(node->left, offset);
    } else if (offset > node->key) {
      node->right = remove_rec(node->right, offset);
    } else {
      // Found the node to remove
      if (!node->left) {
        BSTNode *r = node->right;
        delete node;
        return r;
      }
      if (!node->right) {
        BSTNode *l = node->left;
        delete node;
        return l;
      }
      // Two children: replace with inorder successor
      BSTNode *succ = find_min(node->right);
      node->key = succ->key;
      node->object_id = succ->object_id;
      node->right = remove_rec(node->right, succ->key);
    }
    return node;
  }

  BSTNode *find_rec(BSTNode *node, size_t offset) const {
    if (!node)
      return nullptr;
    if (offset == node->key)
      return node;
    if (offset < node->key)
      return find_rec(node->left, offset);
    return find_rec(node->right, offset);
  }

  void clear_rec(BSTNode *node) {
    if (!node)
      return;
    clear_rec(node->left);
    clear_rec(node->right);
    delete node;
  }

  void inorder_rec(BSTNode *node, std::vector<int> &result) const {
    if (!node)
      return;
    inorder_rec(node->left, result);
    result.push_back(node->object_id);
    inorder_rec(node->right, result);
  }

  std::string node_to_json(BSTNode *node) const {
    if (!node)
      return "null";
    std::ostringstream ss;
    ss << "{\"id\":" << node->object_id << ",\"key\":" << node->key
       << ",\"left\":" << node_to_json(node->left)
       << ",\"right\":" << node_to_json(node->right) << "}";
    return ss.str();
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// GCEngine — The main Garbage Collection engine
//
// Manages the heap, object table, BST, and implements the full
// Mark-Sweep-Compact cycle with LISP2 compaction.
// ─────────────────────────────────────────────────────────────────────────────
class GCEngine {
public:
  // ── Heap ──
  uint8_t heap[HEAP_SIZE]; // Raw memory: 0 = free, object_id = occupied

  // ── Object Table ──
  std::map<int, HeapObject> objects; // id → HeapObject

  // ── BST (sorted by offset) ──
  ObjectBST bst;

  // ── Counters / Stats ──
  int next_id;
  int total_allocations;
  int total_collections;
  int total_compactions;
  int objects_collected;
  size_t bytes_freed;

  // ── GC phase tracking ──
  std::string gc_phase; // "idle" | "marking" | "sweeping" | "compacting"

  // ── Operation log ──
  std::vector<std::string> gc_log;

  // ────────────────────── Constructor / Reset ──────────────────────

  GCEngine() { reset(); }

  void reset() {
    std::memset(heap, 0, HEAP_SIZE);
    objects.clear();
    bst.clear();
    next_id = 1;
    total_allocations = 0;
    total_collections = 0;
    total_compactions = 0;
    objects_collected = 0;
    bytes_freed = 0;
    gc_phase = "idle";
    gc_log.clear();
    log("Heap initialized (" + std::to_string(HEAP_SIZE) + " bytes)");
  }

  int find_free_block(size_t size) const {
    // Collect occupied ranges and sort by offset
    std::vector<std::pair<size_t, size_t>> occupied;
    for (auto &kv : objects) {
      occupied.push_back({kv.second.offset, kv.second.size});
    }
    std::sort(occupied.begin(), occupied.end());

    // Scan gaps between objects
    size_t pos = 0;
    for (auto &range : occupied) {
      if (range.first > pos && range.first - pos >= size)
        return (int)pos;
      pos = range.first + range.second;
    }
    // Gap at the end
    if (HEAP_SIZE - pos >= size)
      return (int)pos;

    return -1;
  }

  int allocate(size_t size, const std::string &type_name, bool is_root) {
    if (size == 0 || size > (size_t)HEAP_SIZE)
      return -1;

    int offset = find_free_block(size);
    if (offset < 0)
      return -1;

    int id = next_id++;
    HeapObject obj(id, type_name, (size_t)offset, size, is_root);
    objects[id] = obj;

    // Paint heap cells with this object's ID marker
    for (size_t i = 0; i < size; i++)
      heap[offset + i] = (uint8_t)(id & 0xFF);

    // Track in BST
    bst.insert(id, (size_t)offset);

    total_allocations++;
    log("Allocated '" + type_name + "' (id=" + std::to_string(id) + ", " +
        std::to_string(size) + "B) at offset " + std::to_string(offset));
    return id;
  }

  int allocate_with_gc(size_t size, const std::string &type_name,
                       bool is_root) {
    int result = allocate(size, type_name, is_root);
    if (result >= 0)
      return result;

    size_t total_free = get_total_free();
    if (total_free >= size) {
      log("⚠ Allocation failed — fragmentation! (need " + std::to_string(size) +
          "B contiguous, largest gap=" +
          std::to_string(get_largest_free_block()) +
          "B, total free=" + std::to_string(total_free) + "B). Running GC…");
      run_gc();
      result = allocate(size, type_name, is_root);
      if (result >= 0)
        log("✓ Allocation succeeded after compaction!");
      else
        log("✗ Allocation still failed after compaction (not enough total free "
            "memory).");
    }
    return result;
  }

  // ────────────────────── Reference Graph ──────────────────────

  bool add_reference(int from_id, int to_id) {
    if (objects.find(from_id) == objects.end() ||
        objects.find(to_id) == objects.end())
      return false;
    // No self-references, no duplicates
    if (from_id == to_id)
      return false;
    auto &refs = objects[from_id].references;
    for (int r : refs)
      if (r == to_id)
        return true;

    refs.push_back(to_id);
    log("Reference: " + objects[from_id].type_name + "[" +
        std::to_string(from_id) + "] → " + objects[to_id].type_name + "[" +
        std::to_string(to_id) + "]");
    return true;
  }

  void remove_root(int object_id) {
    auto it = objects.find(object_id);
    if (it != objects.end()) {
      it->second.is_root = false;
      log("Removed root: " + it->second.type_name + "[" +
          std::to_string(object_id) + "]");
    }
  }

  // ════════════════════════════════════════════════════════════════
  //  MARK PHASE — Stack-based DFS from root set
  // ════════════════════════════════════════════════════════════════

  void mark_phase() {
    gc_phase = "marking";

    // Clear all mark bits
    for (auto &kv : objects)
      kv.second.marked = false;

    // Stack-based reachability traversal
    std::stack<int> mark_stack;
    for (auto &kv : objects) {
      if (kv.second.is_root)
        mark_stack.push(kv.first);
    }

    int marked_count = 0;
    while (!mark_stack.empty()) {
      int obj_id = mark_stack.top();
      mark_stack.pop();

      auto it = objects.find(obj_id);
      if (it == objects.end() || it->second.marked)
        continue;

      it->second.marked = true;
      marked_count++;

      for (int ref_id : it->second.references) {
        auto ref_it = objects.find(ref_id);
        if (ref_it != objects.end() && !ref_it->second.marked)
          mark_stack.push(ref_id);
      }
    }

    log("Mark phase: " + std::to_string(marked_count) + " objects reachable");
  }

  // ════════════════════════════════════════════════════════════════
  //  SWEEP PHASE — Remove unmarked (dead) objects
  // ════════════════════════════════════════════════════════════════

  int sweep_phase() {
    gc_phase = "sweeping";
    std::vector<int> dead;
    size_t freed = 0;

    for (auto &kv : objects) {
      if (!kv.second.marked) {
        dead.push_back(kv.first);
        freed += kv.second.size;
      }
    }

    for (int id : dead) {
      HeapObject &obj = objects[id];
      // Clear heap memory
      for (size_t i = 0; i < obj.size; i++)
        heap[obj.offset + i] = 0;
      // Remove from BST
      bst.remove(obj.offset);
      // Also clean up any dangling references in surviving objects
      for (auto &kv : objects) {
        auto &refs = kv.second.references;
        refs.erase(std::remove(refs.begin(), refs.end(), id), refs.end());
      }
      objects.erase(id);
    }

    objects_collected += (int)dead.size();
    bytes_freed += freed;

    log("Sweep phase: " + std::to_string(dead.size()) +
        " dead objects removed, " + std::to_string(freed) + "B freed");
    return (int)dead.size();
  }

  // ════════════════════════════════════════════════════════════════
  //  COMPACT PHASE — LISP2 Compaction Algorithm
  //
  //  The LISP2 compactor works in three passes:
  //    Pass 1 — Compute forwarding addresses
  //             Walk objects in address order; assign each a new
  //             contiguous offset starting from 0.
  //    Pass 2 — Update inter-object references
  //             (Since we use IDs rather than raw pointers, reference
  //              IDs don't change — but offsets do.  We demonstrate
  //              the pointer-adjustment concept by re-mapping offsets.)
  //    Pass 3 — Relocate objects
  //             Copy each object's data to its forwarding address.
  //             Rebuild the BST with new offsets.
  // ════════════════════════════════════════════════════════════════

  void compact_phase() {
    gc_phase = "compacting";

    // Sorted live objects via BST in-order traversal
    std::vector<int> sorted_ids = bst.inorder();
    if (sorted_ids.empty()) {
      log("Compact phase: nothing to compact");
      gc_phase = "idle";
      return;
    }

    // ── Pass 1: Compute forwarding addresses ──
    size_t free_ptr = 0;
    for (int id : sorted_ids) {
      objects[id].forward_addr = free_ptr;
      free_ptr += objects[id].size;
    }
    log("Compact pass 1: forwarding addresses computed");

    // ── Pass 2: Update references (pointer adjustment) ──
    // In our model, references use object IDs (not raw pointers),
    // so the reference IDs themselves don't change.  However, the
    // *offsets* embedded in each HeapObject must be relocated.
    // This mirrors real compactors updating interior pointers.
    log("Compact pass 2: pointer adjustment (offset relocation)");

    // ── Pass 3: Relocate objects to new positions ──
    uint8_t temp_heap[HEAP_SIZE];
    std::memset(temp_heap, 0, HEAP_SIZE);

    for (int id : sorted_ids) {
      HeapObject &obj = objects[id];
      std::memcpy(temp_heap + obj.forward_addr, heap + obj.offset, obj.size);
    }
    std::memcpy(heap, temp_heap, HEAP_SIZE);

    // Rebuild BST with updated offsets
    bst.clear();
    for (int id : sorted_ids) {
      HeapObject &obj = objects[id];
      obj.offset = obj.forward_addr;
      obj.forward_addr = SIZE_MAX;
      bst.insert(id, obj.offset);

      // Repaint heap cells
      for (size_t i = 0; i < obj.size; i++)
        heap[obj.offset + i] = (uint8_t)(id & 0xFF);
    }

    total_compactions++;
    log("Compact pass 3: " + std::to_string(sorted_ids.size()) +
        " objects relocated — memory defragmented");
    gc_phase = "idle";
  }

  // ════════════════════════════════════════════════════════════════
  //  Full GC Cycle: Mark → Sweep → Compact
  // ════════════════════════════════════════════════════════════════

  void run_gc() {
    log("═══ GC Cycle Started ═══");
    total_collections++;
    mark_phase();
    sweep_phase();
    compact_phase();
    log("═══ GC Cycle Complete ═══");
  }

  // ────────────────────── Demo Scenario ──────────────────────
  // Creates a fragmented heap to demonstrate the "unique feature":
  // total free memory is sufficient, but no single gap is large enough.

  void demo_fragmentation() {
    reset();
    log("═══ Fragmentation Demo Setup ═══");

    // Allocate objects filling the heap in an alternating live/dead pattern:
    // Total heap = 512B. Live = 300B, Dead = 212B across 6 gaps (max gap = 40B).
    int a = allocate(40, "Array", true);    // Live
    int b = allocate(32, "Buffer", true);   // Dead
    int c = allocate(48, "HashMap", true);  // Live
    int d = allocate(36, "String", true);   // Dead
    int e = allocate(32, "Vector", true);   // Live
    int f = allocate(40, "Closure", true);  // Dead
    int g = allocate(44, "Promise", true);  // Live
    int h = allocate(36, "RegExp", true);   // Dead
    int i = allocate(40, "WeakMap", true);  // Live
    int j = allocate(32, "Symbol", true);   // Dead
    int k = allocate(48, "Iterator", true); // Live
    int l = allocate(36, "Proxy", true);    // Dead
    int m = allocate(48, "Anchor", true);   // Live (pins the end of the heap)

    // Wire up references among surviving live objects
    if (a > 0 && e > 0) add_reference(a, e);
    if (c > 0 && g > 0) add_reference(c, g);
    if (g > 0 && i > 0) add_reference(g, i);
    if (k > 0 && m > 0) add_reference(k, m);

    // Remove roots for dead objects: b, d, f, h, j, l
    if (b > 0) remove_root(b);
    if (d > 0) remove_root(d);
    if (f > 0) remove_root(f);
    if (h > 0) remove_root(h);
    if (j > 0) remove_root(j);
    if (l > 0) remove_root(l);

    log("Demo setup complete: 13 objects allocated across 512B heap.");
    log("Garbage objects b, d, f, h, j, l (total 212B) alternate with live objects.");
    log("After sweeping, total free memory is 212B, but the largest gap is only 40B!");
    log("Allocating 80B will FAIL until you run Compaction.");
  }

  // ────────────────────── Random Fill ──────────────────────

  void random_fill(int count) {
    static const char *types[] = {
        "String",   "Array",  "HashMap",    "Buffer",   "Closure",
        "Promise",  "RegExp", "Vector",     "WeakMap",  "Symbol",
        "Iterator", "Proxy",  "TypedArray", "DataView", "Set"};
    if (count <= 0)
      count = 5;

    for (int n = 0; n < count; n++) {
      int sz = 8 + std::rand() % 56; // 8–63 bytes
      int tIdx = std::rand() % 15;
      bool root = (std::rand() % 3 != 0); // ~66 % root
      allocate((size_t)sz, types[tIdx], root);
    }

    // Sprinkle random inter-object references
    std::vector<int> ids;
    for (auto &kv : objects)
      ids.push_back(kv.first);
    if (ids.size() >= 2) {
      for (int n = 0; n < count / 2 + 1; n++) {
        int from = ids[std::rand() % ids.size()];
        int to = ids[std::rand() % ids.size()];
        if (from != to)
          add_reference(from, to);
      }
    }
  }

  // ────────────────────── Statistics ──────────────────────

  size_t get_total_used() const {
    size_t used = 0;
    for (auto &kv : objects)
      used += kv.second.size;
    return used;
  }

  size_t get_total_free() const { return (size_t)HEAP_SIZE - get_total_used(); }

  size_t get_largest_free_block() const {
    std::vector<std::pair<size_t, size_t>> occupied;
    for (auto &kv : objects)
      occupied.push_back({kv.second.offset, kv.second.size});
    std::sort(occupied.begin(), occupied.end());

    size_t largest = 0, pos = 0;
    for (auto &r : occupied) {
      if (r.first > pos)
        largest = std::max(largest, r.first - pos);
      pos = r.first + r.second;
    }
    largest = std::max(largest, (size_t)HEAP_SIZE - pos);
    return largest;
  }

  double get_fragmentation() const {
    size_t total_free = get_total_free();
    if (total_free == 0)
      return 0.0;
    size_t largest = get_largest_free_block();
    return 1.0 - (double)largest / (double)total_free;
  }

  int get_fragment_count() const {
    std::vector<std::pair<size_t, size_t>> occupied;
    for (auto &kv : objects)
      occupied.push_back({kv.second.offset, kv.second.size});
    std::sort(occupied.begin(), occupied.end());

    int frags = 0;
    size_t pos = 0;
    for (auto &r : occupied) {
      if (r.first > pos)
        frags++;
      pos = r.first + r.second;
    }
    if ((size_t)HEAP_SIZE > pos)
      frags++;
    return frags;
  }

  // ────────────────────── JSON Serialization ──────────────────────
  // Produces the full engine state as a JSON string for the frontend.

  std::string get_state_json() const {
    std::ostringstream ss;
    ss << "{";

    // ── Heap ──
    ss << "\"heap\":{\"size\":" << HEAP_SIZE << ",\"cells\":[";
    for (int i = 0; i < HEAP_SIZE; i++) {
      if (i)
        ss << ",";
      ss << (int)heap[i];
    }
    ss << "],\"used\":" << get_total_used() << ",\"free\":" << get_total_free()
       << ",\"largest_free\":" << get_largest_free_block()
       << ",\"fragmentation\":" << get_fragmentation()
       << ",\"fragment_count\":" << get_fragment_count() << "},";

    // ── Objects ──
    ss << "\"objects\":[";
    {
      bool first = true;
      for (auto &kv : objects) {
        if (!first)
          ss << ",";
        first = false;
        const HeapObject &o = kv.second;
        ss << "{\"id\":" << o.id << ",\"type\":\"" << esc(o.type_name) << "\""
           << ",\"offset\":" << o.offset << ",\"size\":" << o.size
           << ",\"marked\":" << (o.marked ? "true" : "false")
           << ",\"is_root\":" << (o.is_root ? "true" : "false")
           << ",\"forward_addr\":"
           << (o.forward_addr == SIZE_MAX ? -1 : (long long)o.forward_addr)
           << ",\"references\":[";
        for (size_t ri = 0; ri < o.references.size(); ri++) {
          if (ri)
            ss << ",";
          ss << o.references[ri];
        }
        ss << "]}";
      }
    }
    ss << "],";

    // ── BST ──
    ss << "\"bst\":" << bst.to_json() << ",";

    // ── Stats ──
    ss << "\"stats\":{"
       << "\"total_allocations\":" << total_allocations
       << ",\"total_collections\":" << total_collections
       << ",\"total_compactions\":" << total_compactions
       << ",\"objects_collected\":" << objects_collected
       << ",\"bytes_freed\":" << bytes_freed
       << ",\"live_objects\":" << objects.size() << "},";

    // ── GC Phase ──
    ss << "\"gc_phase\":\"" << gc_phase << "\",";

    // ── Log (last 100 entries) ──
    ss << "\"log\":[";
    {
      size_t start = gc_log.size() > 100 ? gc_log.size() - 100 : 0;
      for (size_t i = start; i < gc_log.size(); i++) {
        if (i > start)
          ss << ",";
        ss << "\"" << esc(gc_log[i]) << "\"";
      }
    }
    ss << "]";

    ss << "}";
    return ss.str();
  }

private:
  void log(const std::string &msg) { gc_log.push_back(msg); }

  static std::string esc(const std::string &s) {
    std::string r;
    for (char c : s) {
      switch (c) {
      case '"':
        r += "\\\"";
        break;
      case '\\':
        r += "\\\\";
        break;
      case '\n':
        r += "\\n";
        break;
      case '\r':
        r += "\\r";
        break;
      case '\t':
        r += "\\t";
        break;
      default:
        r += c;
      }
    }
    return r;
  }
};
