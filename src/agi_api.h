#pragma once
#ifndef AGI_API_H
#define AGI_API_H

#include <string>
#include <memory>
#include <functional>
#include "web_server.h"
#include "state.h"

class AGI_API {
public:
    explicit AGI_API(int port = 8080);
    ~AGI_API();
    
    void start();
    void stop();

private:
    std::unique_ptr<WebServer> server_;
    
    HttpResponse handle_chat(const HttpRequest& req);
    HttpResponse handle_status(const HttpRequest& req);
    HttpResponse handle_consciousness(const HttpRequest& req);
    HttpResponse handle_thoughts(const HttpRequest& req);
    HttpResponse handle_memory(const HttpRequest& req);
    HttpResponse handle_goals(const HttpRequest& req);
    HttpResponse handle_valence(const HttpRequest& req);
    HttpResponse handle_history(const HttpRequest& req);
    HttpResponse handle_save(const HttpRequest& req);
    HttpResponse handle_load(const HttpRequest& req);
    HttpResponse handle_clear(const HttpRequest& req);
    HttpResponse handle_ui(const HttpRequest& req);
    
    std::string json_escape(const std::string& str);
    std::string filter_markers(const std::string& text);
    std::string sanitize_output(const std::string& raw);
};

#endif // AGI_API_H
