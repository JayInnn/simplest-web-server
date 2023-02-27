#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

#include "../../logger/log.h"

class http_request {

public:
    enum HTTP_METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };

public:
    http_request(){}
    ~http_request() = default;
    http_request(const http_request&) = delete;
    http_request& operator=(const http_request&) = delete;

    void set_request_line(std::string method, std::string path, std::string version);
    void set_requset_header(std::string key, std::string value);
    void set_request_body(std::string body);
    std::string get_path() const;
    bool get_keepalive() const;
    HTTP_METHOD get_method() const;


private:
    void parse_from_urlencoded();
    bool user_verify(const std::string &name, const std::string &pwd, bool isLogin);
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

private:
    HTTP_METHOD req_method;
    std::string req_path;
    std::string req_version;
    std::unordered_map<std::string, std::string> req_header;
    std::unordered_map<std::string, std::string> req_post;
    std::string req_body;
};


#endif