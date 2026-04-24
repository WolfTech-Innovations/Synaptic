#include "web_server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>
    #include <fcntl.h>
    #include <sys/select.h>
    #include <errno.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

WebServer::WebServer(int port) : port_(port), running_(false), listen_socket_(INVALID_SOCKET) {}

WebServer::~WebServer() {
    stop();
}

void WebServer::start() {
    if (running_.load()) return;
    running_.store(true);
    server_thread_ = std::make_unique<std::thread>(&WebServer::run_server, this);
}

void WebServer::stop() {
    if (!running_.load()) return;
    
    running_.store(false);
    
    // Close socket to interrupt accept/select
    if (listen_socket_ != INVALID_SOCKET) {
        shutdown(listen_socket_, SHUT_RDWR);
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }
    
    // Wait for thread
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
}

bool WebServer::is_running() const {
    return running_.load();
}

void WebServer::register_route(const std::string& method, const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    if (method == "GET") {
        get_handlers_[path] = handler;
    } else if (method == "POST") {
        post_handlers_[path] = handler;
    }
}

void WebServer::register_static_file(const std::string& path, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    static_files_[path] = file_path;
}

void WebServer::run_server() {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        running_.store(false);
        return;
    }
    
    listen_socket_ = sock;

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        closesocket(sock);
        listen_socket_ = INVALID_SOCKET;
        running_.store(false);
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_);

    if (::bind(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port_ << std::endl;
        closesocket(sock);
        listen_socket_ = INVALID_SOCKET;
        running_.store(false);
        return;
    }

    if (::listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(sock);
        listen_socket_ = INVALID_SOCKET;
        running_.store(false);
        return;
    }

    std::cout << "WebServer started on port " << port_ << std::endl;

    while (running_.load()) {
        SOCKET current_sock = listen_socket_;
        if (current_sock == INVALID_SOCKET) break;

#ifndef _WIN32
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(current_sock, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int select_result = select(current_sock + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (select_result < 0) {
            if (errno == EBADF || errno == EINTR) break;
            std::cerr << "Select error: " << errno << std::endl;
            break;
        }
        
        if (select_result == 0 || !running_.load()) {
            continue;
        }
        
        if (!FD_ISSET(current_sock, &read_fds)) {
            continue;
        }
#endif

        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(current_sock, (sockaddr*)&client_addr, &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
            if (!running_.load()) break;
            continue;
        }

        char buffer[8192] = {0};
        int recv_len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            try {
                HttpRequest req = parse_request(std::string(buffer));
                HttpResponse resp = handle_request(req);
                std::string response_str = serialize_response(resp);
                send(client_socket, response_str.c_str(), response_str.length(), 0);
            } catch (const std::exception& e) {
                std::cerr << "Request handling error: " << e.what() << std::endl;
            }
        }

        closesocket(client_socket);
    }

    if (listen_socket_ != INVALID_SOCKET) {
        shutdown(listen_socket_, SHUT_RDWR);
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

HttpRequest WebServer::parse_request(const std::string& raw_request) {
    HttpRequest req;
    if (raw_request.empty()) return req;
    
    std::istringstream iss(raw_request);
    
    std::string request_line;
    if (!std::getline(iss, request_line)) return req;
    
    std::istringstream line_iss(request_line);
    line_iss >> req.method >> req.path;
    
    size_t query_pos = req.path.find('?');
    if (query_pos != std::string::npos) {
        req.query = req.path.substr(query_pos + 1);
        req.path = req.path.substr(0, query_pos);
    }
    
    std::string header_line;
    while (std::getline(iss, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        if (header_line.empty()) break;
        
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos && colon_pos + 2 < header_line.length()) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 2);
            req.headers[key] = value;
        }
    }
    
    std::string remaining((std::istreambuf_iterator<char>(iss)), std::istreambuf_iterator<char>());
    req.body = remaining;
    
    return req;
}

HttpResponse WebServer::handle_request(const HttpRequest& req) {
    HttpResponse resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.headers["Access-Control-Allow-Origin"] = "*";
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    resp.headers["Access-Control-Allow-Headers"] = "Content-Type";
    
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    if (req.method == "GET") {
        auto it = get_handlers_.find(req.path);
        if (it != get_handlers_.end()) {
            resp = it->second(req);
        } else {
            resp.status_code = 404;
            resp.body = "{\"error\": \"Not found\"}";
        }
    } else if (req.method == "POST") {
        auto it = post_handlers_.find(req.path);
        if (it != post_handlers_.end()) {
            resp = it->second(req);
        } else {
            resp.status_code = 404;
            resp.body = "{\"error\": \"Not found\"}";
        }
    } else if (req.method == "OPTIONS") {
        resp.status_code = 200;
        resp.body = "";
    } else {
        resp.status_code = 405;
        resp.body = "{\"error\": \"Method not allowed\"}";
    }
    
    return resp;
}

std::string WebServer::serialize_response(const HttpResponse& resp) {
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << resp.status_code << " OK\r\n";
    oss << "Server: NexusAGI/1.0\r\n";
    oss << "Content-Length: " << resp.body.length() << "\r\n";
    
    for (const auto& [key, value] : resp.headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n" << resp.body;
    
    return oss.str();
}

std::string WebServer::url_decode(const std::string& url) {
    std::string decoded;
    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%' && i + 2 < url.length()) {
            try {
                std::string hex = url.substr(i + 1, 2);
                decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
                i += 2;
            } catch (...) {
                decoded += url[i];
            }
        } else if (url[i] == '+') {
            decoded += ' ';
        } else {
            decoded += url[i];
        }
    }
    return decoded;
}
