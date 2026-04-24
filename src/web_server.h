#pragma once
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
};

using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

class WebServer {
public:
    WebServer(int port = 8080);
    ~WebServer();
    
    void start();
    void stop();
    bool is_running() const;
    
    void register_route(const std::string& method, const std::string& path, RequestHandler handler);
    void register_static_file(const std::string& path, const std::string& file_path);
    
private:
    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    std::mutex handlers_mutex_;
    std::map<std::string, RequestHandler> get_handlers_;
    std::map<std::string, RequestHandler> post_handlers_;
    std::map<std::string, std::string> static_files_;
    
    int listen_socket_;  // No atomic needed - protected by running_ flag
    
    void run_server();
    HttpResponse handle_request(const HttpRequest& req);
    HttpRequest parse_request(const std::string& raw_request);
    std::string serialize_response(const HttpResponse& resp);
    std::string url_decode(const std::string& url);
};

#endif // WEB_SERVER_H
