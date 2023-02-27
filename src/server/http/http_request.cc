#include <sstream>

#include "http_request.h"
#include "../../utils/local_cache.h"

const std::unordered_map<std::string, int> http_request::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1}};

const std::unordered_set<std::string> http_request::DEFAULT_HTML{
            "/index", "/register", "/login","/welcome", "/video", "/picture"};


void http_request::set_request_line(std::string method, std::string path, std::string version) {
    if(method == "GET") {
        req_method = GET;
    } else if(method == "POST") {
        req_method = POST;
    }
    req_version = version;
    req_path = path;
    if(path == "/") {
        req_path = "/index.html"; 
    } else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path) {
                req_path += ".html";
                break;
            }
        }
    }
}

void http_request::set_requset_header(std::string key, std::string value) {
    req_header[key] = value;
}

void http_request::set_request_body(std::string body) {
    req_body = body;
    if(req_method == POST && req_header["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_from_urlencoded();
        if(DEFAULT_HTML_TAG.count(req_path)) {
            int tag = DEFAULT_HTML_TAG.find(req_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(user_verify(req_post["username"], req_post["password"], isLogin)) {
                    req_path = "/welcome.html";
                }else {
                    req_path = "/error.html";
                }
            }
        }
    }  
}

std::string http_request::get_path() const {
    return req_path;
}

bool http_request::get_keepalive() const {
    if(req_header.count("Connection") == 1) {
        return req_header.find("Connection")->second == "keep-alive" && req_version == "1.1";
    }
    return false;
}

http_request::HTTP_METHOD http_request::get_method() const {
    return req_method;
}


/**
 * private method
 */
void http_request::parse_from_urlencoded() {
    if(req_body.size() == 0) {
        return;
    }

    std::string key, value;
    int num = 0;
    int n = req_body.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = req_body[i];
        switch (ch) {
            case '=':
                key = req_body.substr(j, i - j);
                j = i + 1;
                break;
            case '&':
                value = req_body.substr(j, i - j);
                j = i + 1;
                req_post[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if(req_post.count(key) == 0 && j < i) {
        value = req_body.substr(j, i - j);
        req_post[key] = value;
    }
}


bool http_request::user_verify(const std::string &name, const std::string &pwd, bool is_login) {
    if(name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("verify name:%s pwd:%s", name.c_str(), pwd.c_str());

    std::stringstream verify_msg;
    for(auto pair:database_local_cache::verify_msg_map) {
        verify_msg << "[" << pair.first << "|" << pair.second << "]    ";
    }
    LOG_INFO("verify msg map: %s", verify_msg.str().c_str());

    if(is_login) {
        // 登入
        LOG_INFO("user login, name: %s", name.c_str());
        if(database_local_cache::verify_msg_map.count(name) == 1 && 
            database_local_cache::verify_msg_map[name] == pwd) {
            return true;
        }
    } else {
        // 注册
        LOG_INFO("register new user, name: %s", name.c_str());
        if(database_local_cache::verify_msg_map.count(name) == 0) {
            database_local_cache::verify_msg_map[name] = pwd;
            return true;
        }
    }
    return false;
}