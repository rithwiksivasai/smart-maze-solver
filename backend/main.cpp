#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSE_SOCKET close
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct Point { int r; int c; };
struct Result {
    std::vector<Point> visited;
    std::vector<Point> path;
    double elapsedMs = 0;
    bool found = false;
};

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string jsonString(const std::string& text, const std::string& key,
                       const std::string& fallback = {}) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? match[1].str() : fallback;
}

int jsonInt(const std::string& text, const std::string& key, int fallback) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? std::stoi(match[1].str()) : fallback;
}

Point jsonPoint(const std::string& text, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\{[^}]*\\}");
    std::smatch object;
    if (!std::regex_search(text, object, pattern)) return {-1, -1};
    return {jsonInt(object.str(), "r", -1), jsonInt(object.str(), "c", -1)};
}

std::vector<std::string> jsonGrid(const std::string& text) {
    const std::regex gridPattern("\\\"grid\\\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch gridMatch;
    if (!std::regex_search(text, gridMatch, gridPattern)) return {};
    const std::regex rowPattern("\\\"([.#]+)\\\"");
    const std::string body = gridMatch[1].str();
    std::vector<std::string> rows;
    for (auto it = std::sregex_iterator(body.begin(), body.end(), rowPattern);
         it != std::sregex_iterator(); ++it) rows.push_back((*it)[1].str());
    return rows;
}

std::string pointList(const std::vector<Point>& points) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i) out << ',';
        out << '[' << points[i].r << ',' << points[i].c << ']';
    }
    out << ']';
    return out.str();
}

Result solve(const std::vector<std::string>& grid, Point start, Point goal,
             const std::string& algorithm) {
    Result result;
    const auto begin = std::chrono::steady_clock::now();
    const int rows = static_cast<int>(grid.size());
    const int cols = rows ? static_cast<int>(grid.front().size()) : 0;
    if (!rows || !cols || start.r < 0 || start.r >= rows || start.c < 0 || start.c >= cols ||
        goal.r < 0 || goal.r >= rows || goal.c < 0 || goal.c >= cols) return result;

    const int total = rows * cols;
    const int source = start.r * cols + start.c;
    const int target = goal.r * cols + goal.c;
    std::vector<int> parent(total, -1), distance(total, total + 1);
    std::vector<bool> seen(total, false);
    const int dr[] = {-1, 0, 1, 0};
    const int dc[] = {0, 1, 0, -1};
    auto heuristic = [=](int id) {
        return std::abs(id / cols - goal.r) + std::abs(id % cols - goal.c);
    };

    if (algorithm == "DFS") {
        std::vector<int> stack{source};
        seen[source] = true;
        while (!stack.empty()) {
            const int current = stack.back(); stack.pop_back();
            result.visited.push_back({current / cols, current % cols});
            if (current == target) { result.found = true; break; }
            for (int d = 3; d >= 0; --d) {
                const int nr = current / cols + dr[d], nc = current % cols + dc[d];
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols || grid[nr][nc] == '#' ) continue;
                const int next = nr * cols + nc;
                if (!seen[next]) { seen[next] = true; parent[next] = current; stack.push_back(next); }
            }
        }
    } else if (algorithm == "BFS") {
        std::queue<int> queue;
        queue.push(source); seen[source] = true;
        while (!queue.empty()) {
            const int current = queue.front(); queue.pop();
            result.visited.push_back({current / cols, current % cols});
            if (current == target) { result.found = true; break; }
            for (int d = 0; d < 4; ++d) {
                const int nr = current / cols + dr[d], nc = current % cols + dc[d];
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols || grid[nr][nc] == '#') continue;
                const int next = nr * cols + nc;
                if (!seen[next]) { seen[next] = true; parent[next] = current; queue.push(next); }
            }
        }
    } else {
        using Entry = std::pair<int, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
        distance[source] = 0;
        queue.push({algorithm == "A*" ? heuristic(source) : 0, source});
        while (!queue.empty()) {
            const int current = queue.top().second; queue.pop();
            if (seen[current]) continue;
            seen[current] = true;
            result.visited.push_back({current / cols, current % cols});
            if (current == target) { result.found = true; break; }
            for (int d = 0; d < 4; ++d) {
                const int nr = current / cols + dr[d], nc = current % cols + dc[d];
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols || grid[nr][nc] == '#') continue;
                const int next = nr * cols + nc;
                const int candidate = distance[current] + 1;
                if (candidate < distance[next]) {
                    distance[next] = candidate;
                    parent[next] = current;
                    queue.push({candidate + (algorithm == "A*" ? heuristic(next) : 0), next});
                }
            }
        }
    }

    if (result.found) {
        for (int at = target; at != -1; at = parent[at]) result.path.push_back({at / cols, at % cols});
        std::reverse(result.path.begin(), result.path.end());
    }
    result.elapsedMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - begin).count();
    return result;
}

std::string mimeType(const std::string& path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html; charset=utf-8";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") return "text/css; charset=utf-8";
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

bool sendAll(socket_t client, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int count = send(client, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (count <= 0) return false;
        sent += static_cast<std::size_t>(count);
    }
    return true;
}

void respond(socket_t client, const std::string& status, const std::string& type,
             const std::string& body) {
    std::ostringstream header;
    header << "HTTP/1.1 " << status << "\r\nContent-Type: " << type
           << "\r\nContent-Length: " << body.size()
           << "\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
    sendAll(client, header.str() + body);
}

void handleClient(socket_t client) {
    std::string request;
    char buffer[4096];
    int count = 0;
    std::size_t headerEnd = std::string::npos;
    std::size_t contentLength = 0;
    while ((count = recv(client, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, count);
        headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            const std::string headers = request.substr(0, headerEnd);
            std::smatch lengthMatch;
            if (std::regex_search(headers, lengthMatch, std::regex("Content-Length:\\s*(\\d+)", std::regex::icase)))
                contentLength = static_cast<std::size_t>(std::stoul(lengthMatch[1].str()));
            if (request.size() >= headerEnd + 4 + contentLength) break;
        }
        if (request.size() > 2 * 1024 * 1024) break;
    }
    if (headerEnd == std::string::npos) { CLOSE_SOCKET(client); return; }
    const std::string headers = request.substr(0, headerEnd);
    const std::string body = request.substr(headerEnd + 4);
    std::istringstream firstLine(request.substr(0, request.find("\r\n")));
    std::string method, path, version;
    firstLine >> method >> path >> version;

    if (method == "OPTIONS") {
        respond(client, "204 No Content", "text/plain", "");
    } else if (path == "/api/solve" && method == "POST") {
        const auto grid = jsonGrid(body);
        const Point start = jsonPoint(body, "start"), goal = jsonPoint(body, "goal");
        const std::string algorithm = jsonString(body, "algorithm", "A*");
        bool validGrid = !grid.empty() && grid.size() <= 151;
        for (const auto& row : grid) validGrid = validGrid && row.size() == grid.front().size() && row.size() <= 151;
        if (!validGrid || grid.size() * grid.front().size() > 10000) {
            respond(client, "400 Bad Request", "application/json", "{\"error\":\"Invalid maze grid\"}");
        } else {
            const Result result = solve(grid, start, goal, algorithm);
            std::ostringstream json;
            json << "{\"found\":" << (result.found ? "true" : "false")
                 << ",\"visited\":" << pointList(result.visited)
                 << ",\"path\":" << pointList(result.path)
                 << ",\"visitedCount\":" << result.visited.size()
                 << ",\"pathLength\":" << (result.path.empty() ? 0 : result.path.size() - 1)
                 << ",\"elapsedMs\":" << result.elapsedMs << '}';
            respond(client, "200 OK", "application/json; charset=utf-8", json.str());
        }
    } else if (method == "GET") {
        if (path == "/") path = "/index.html";
        if (path.find("..") != std::string::npos) {
            respond(client, "400 Bad Request", "text/plain", "Bad path");
        } else {
            const std::string file = readFile("web" + path);
            if (file.empty()) respond(client, "404 Not Found", "text/plain", "Not found");
            else respond(client, "200 OK", mimeType(path), file);
        }
    } else {
        respond(client, "404 Not Found", "text/plain", "Not found");
    }
    CLOSE_SOCKET(client);
}
}

int main(int argc, char** argv) {
    const char* portEnv = std::getenv("PORT");
    const int port = argc > 1 ? std::atoi(argv[1]) : (portEnv ? std::atoi(portEnv) : 8080);
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { std::cerr << "Winsock initialization failed\n"; return 1; }
#endif
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) { std::cerr << "Could not create server socket\n"; return 1; }
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<unsigned short>(port));
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR || listen(server, 16) == SOCKET_ERROR) {
        std::cerr << "Could not listen on port " << port << "\n";
        CLOSE_SOCKET(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    std::cout << "Smart Maze Solver running at http://localhost:" << port << "\n";
    while (true) {
        socket_t client = accept(server, nullptr, nullptr);
        if (client != INVALID_SOCKET) handleClient(client);
    }
    CLOSE_SOCKET(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
