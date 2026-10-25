// ============================================================================
// http_server.h — Minimal POSIX HTTP Server (macOS / Linux)
//
// Lightweight, zero-dependency HTTP/1.1 server for serving the frontend
// and exposing the GC engine's REST API.
//
// Features:
//   • GET / POST routing
//   • JSON body parsing helpers
//   • CORS headers for local development
//   • Static file serving (for index.html)
// ============================================================================

#pragma once

#include <string>
#include <functional>
#include <map>
#include <sstream>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

// ─────────────────────────────────────────────────────────────────────────────
// HTTP Request
// ─────────────────────────────────────────────────────────────────────────────
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
};

// ─────────────────────────────────────────────────────────────────────────────
// HTTP Response
// ─────────────────────────────────────────────────────────────────────────────
struct HttpResponse {
    int         status;
    std::string content_type;
    std::string body;

    HttpResponse() : status(200), content_type("text/html") {}

    static HttpResponse json(const std::string& json_body) {
        HttpResponse r;
        r.content_type = "application/json";
        r.body         = json_body;
        return r;
    }

    static HttpResponse html(const std::string& html_body) {
        HttpResponse r;
        r.content_type = "text/html; charset=utf-8";
        r.body         = html_body;
        return r;
    }

    static HttpResponse css(const std::string& css_body) {
        HttpResponse r;
        r.content_type = "text/css; charset=utf-8";
        r.body         = css_body;
        return r;
    }

    static HttpResponse error(int status, const std::string& msg) {
        HttpResponse r;
        r.status       = status;
        r.content_type = "application/json";
        r.body         = "{\"error\":\"" + msg + "\"}";
        return r;
    }

    std::string serialize() const {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << status << " " << status_text() << "\r\n"
           << "Content-Type: "   << content_type << "\r\n"
           << "Content-Length: " << body.size()   << "\r\n"
           << "Access-Control-Allow-Origin: *\r\n"
           << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
           << "Access-Control-Allow-Headers: Content-Type\r\n"
           << "Connection: close\r\n"
           << "\r\n"
           << body;
        return ss.str();
    }

private:
    std::string status_text() const {
        switch (status) {
            case 200: return "OK";
            case 400: return "Bad Request";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            default:  return "Unknown";
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Route handler signature
// ─────────────────────────────────────────────────────────────────────────────
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

// ─────────────────────────────────────────────────────────────────────────────
// HttpServer
// ─────────────────────────────────────────────────────────────────────────────
class HttpServer {
public:
    int  port;
    bool running;

    explicit HttpServer(int port = 8080)
        : port(port), running(false), server_fd(-1) {}

    void get (const std::string& path, RouteHandler handler) { get_routes[path]  = handler; }
    void post(const std::string& path, RouteHandler handler) { post_routes[path] = handler; }

    bool start() {
        signal(SIGPIPE, SIG_IGN);

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "[http] Failed to create socket\n";
            return false;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

        struct sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[http] Failed to bind port " << port << "\n";
            close(server_fd);
            return false;
        }

        if (listen(server_fd, 16) < 0) {
            std::cerr << "[http] Failed to listen\n";
            close(server_fd);
            return false;
        }

        running = true;
        while (running) {
            struct sockaddr_in cli{};
            socklen_t cli_len = sizeof(cli);
            int client_fd = accept(server_fd, (struct sockaddr*)&cli, &cli_len);
            if (client_fd < 0) continue;
            handle_client(client_fd);
            close(client_fd);
        }

        close(server_fd);
        return true;
    }

    void stop() {
        running = false;
        if (server_fd >= 0) close(server_fd);
    }

private:
    int server_fd;
    std::map<std::string, RouteHandler> get_routes;
    std::map<std::string, RouteHandler> post_routes;

    void handle_client(int fd) {
        // Read up to 64 KB
        char buf[65536];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) return;
        buf[n] = '\0';

        HttpRequest req = parse(std::string(buf, (size_t)n));

        // CORS preflight
        if (req.method == "OPTIONS") {
            HttpResponse resp;
            resp.body = "";
            send_response(fd, resp);
            return;
        }

        HttpResponse resp;
        if ((req.method == "GET" || req.method == "HEAD") && get_routes.count(req.path)) {
            resp = get_routes[req.path](req);
            if (req.method == "HEAD") resp.body = "";
        } else if (req.method == "POST" && post_routes.count(req.path)) {
            resp = post_routes[req.path](req);
        } else {
            resp = HttpResponse::error(404, "Not Found");
        }

        send_response(fd, resp);
    }

    void send_response(int fd, const HttpResponse& resp) {
        std::string raw = resp.serialize();
        size_t total = 0;
        while (total < raw.size()) {
            ssize_t sent = write(fd, raw.c_str() + total, raw.size() - total);
            if (sent <= 0) break;
            total += (size_t)sent;
        }
    }

    static HttpRequest parse(const std::string& raw) {
        HttpRequest req;
        std::istringstream ss(raw);
        std::string line;

        // Request line
        if (std::getline(ss, line)) {
            size_t s1 = line.find(' ');
            size_t s2 = (s1 != std::string::npos) ? line.find(' ', s1 + 1) : std::string::npos;
            if (s1 != std::string::npos && s2 != std::string::npos) {
                req.method = line.substr(0, s1);
                req.path   = line.substr(s1 + 1, s2 - s1 - 1);
            }
        }

        // Headers
        int content_length = 0;
        while (std::getline(ss, line)) {
            if (line == "\r" || line.empty()) break;
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                // Trim leading space and trailing \r
                while (!val.empty() && val.front() == ' ') val.erase(val.begin());
                while (!val.empty() && val.back() == '\r')  val.pop_back();
                req.headers[key] = val;
                if (key == "Content-Length") content_length = std::stoi(val);
            }
        }

        // Body
        if (content_length > 0) {
            size_t body_start = raw.find("\r\n\r\n");
            if (body_start != std::string::npos)
                req.body = raw.substr(body_start + 4);
        }

        return req;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Simple JSON parsing helpers (for flat request bodies)
// ─────────────────────────────────────────────────────────────────────────────

inline std::string json_string(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++; // skip opening quote
    size_t end = json.find('"', pos);
    return (end != std::string::npos) ? json.substr(pos, end - pos) : "";
}

inline int json_int(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return -1;
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;
    std::string num;
    while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '-'))
        num += json[pos++];
    return num.empty() ? -1 : std::stoi(num);
}

inline bool json_bool(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;
    return (pos + 4 <= json.size() && json.substr(pos, 4) == "true");
}
