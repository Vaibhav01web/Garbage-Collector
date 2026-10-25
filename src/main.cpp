

#include "gc_engine.h"
#include "http_server.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

// ── Read a file from disk (used to serve frontend/index.html) ──
static std::string read_file(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    return "";
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char *argv[]) {
  int port = 8080;
  if (const char *env_port = std::getenv("PORT")) {
    int p = std::atoi(env_port);
    if (p > 0)
      port = p;
  } else if (argc > 1) {
    port = std::atoi(argv[1]);
  }

  std::srand((unsigned)std::time(nullptr));

  GCEngine engine;
  HttpServer server(port);

  // ══════════════════════════════════════════════════════════════
  //  STATIC FILES
  // ══════════════════════════════════════════════════════════════

  server.get("/", [](const HttpRequest &) {
    std::string html = read_file("frontend/index.html");
    if (html.empty())
      return HttpResponse::error(500, "frontend/index.html not found");
    return HttpResponse::html(html);
  });

  server.get("/index.html", [](const HttpRequest &) {
    std::string html = read_file("frontend/index.html");
    if (html.empty())
      return HttpResponse::error(500, "frontend/index.html not found");
    return HttpResponse::html(html);
  });

  server.get("/api/health", [](const HttpRequest &) {
    return HttpResponse::json("{\"status\":\"ok\",\"service\":\"gc_engine\"}");
  });

  server.get("/docs.pdf", [](const HttpRequest &) {
    std::string pdf = read_file("Memory_Compaction_and_GC_Engine_Documentation.pdf");
    if (pdf.empty())
      return HttpResponse::error(404, "PDF documentation not found");
    HttpResponse r;
    r.content_type = "application/pdf";
    r.body = pdf;
    return r;
  });

  server.get("/style.css", [](const HttpRequest &) {
    std::string css = read_file("frontend/style.css");
    if (css.empty())
      return HttpResponse::error(404, "style.css not found");
    return HttpResponse::css(css);
  });

  // ══════════════════════════════════════════════════════════════
  //  REST API
  // ══════════════════════════════════════════════════════════════

  // ── Get full engine state ──
  server.get("/api/state", [&engine](const HttpRequest &) {
    return HttpResponse::json(engine.get_state_json());
  });

  // ── Allocate a new object ──
  server.post("/api/allocate", [&engine](const HttpRequest &req) {
    int size = json_int(req.body, "size");
    std::string name = json_string(req.body, "name");
    bool is_root = json_bool(req.body, "is_root");
    bool with_gc = json_bool(req.body, "with_gc");

    if (size <= 0)
      size = 24;
    if (name.empty())
      name = "Object";

    int id = with_gc ? engine.allocate_with_gc((size_t)size, name, is_root)
                     : engine.allocate((size_t)size, name, is_root);

    if (id < 0) {
      std::ostringstream ss;
      ss << "{\"success\":false,\"error\":\"Allocation failed\""
         << ",\"total_free\":" << engine.get_total_free()
         << ",\"largest_free\":" << engine.get_largest_free_block() << "}";
      return HttpResponse::json(ss.str());
    }
    return HttpResponse::json("{\"success\":true,\"id\":" + std::to_string(id) +
                              "}");
  });

  // ── Add a reference between objects ──
  server.post("/api/reference", [&engine](const HttpRequest &req) {
    int from = json_int(req.body, "from");
    int to = json_int(req.body, "to");
    bool ok = engine.add_reference(from, to);
    return HttpResponse::json(ok ? "{\"success\":true}"
                                 : "{\"success\":false}");
  });

  // ── Remove root status (make object potentially unreachable) ──
  server.post("/api/remove-root", [&engine](const HttpRequest &req) {
    int id = json_int(req.body, "id");
    engine.remove_root(id);
    return HttpResponse::json("{\"success\":true}");
  });

  // ── Run GC (full or individual phase) ──
  server.post("/api/gc", [&engine](const HttpRequest &req) {
    std::string phase = json_string(req.body, "phase");

    if (phase == "mark")
      engine.mark_phase();
    else if (phase == "sweep")
      engine.sweep_phase();
    else if (phase == "compact")
      engine.compact_phase();
    else
      engine.run_gc();

    return HttpResponse::json(engine.get_state_json());
  });

  // ── Demo fragmentation scenario ──
  server.post("/api/demo", [&engine](const HttpRequest &) {
    engine.demo_fragmentation();
    return HttpResponse::json(engine.get_state_json());
  });

  // ── Reset engine ──
  server.post("/api/reset", [&engine](const HttpRequest &) {
    engine.reset();
    return HttpResponse::json(engine.get_state_json());
  });

  // ── Random fill ──
  server.post("/api/random", [&engine](const HttpRequest &req) {
    int count = json_int(req.body, "count");
    if (count <= 0)
      count = 5;
    engine.random_fill(count);
    return HttpResponse::json(engine.get_state_json());
  });

  // ══════════════════════════════════════════════════════════════
  //  Banner & Start
  // ══════════════════════════════════════════════════════════════

  std::cout << "\n"
               "  ╔═══════════════════════════════════════════════════╗\n"
               "  ║  Memory Compaction & Garbage Collector Engine     ║\n"
               "  ║                                                   ║\n"
               "  ║  Backend : C++ (POSIX sockets)                    ║\n"
               "  ║  Heap    : "
            << HEAP_SIZE
            << " bytes              ║\n"
               "  ║  GC      : Mark-Sweep-Compact (LISP2)             ║\n"
               "  ║                                                   ║\n"
               "  ║  ► http://localhost:"
            << port
            << "                ║\n"
               "  ╚═══════════════════════════════════════════════════╝\n\n";

  server.start();
  return 0;
}
